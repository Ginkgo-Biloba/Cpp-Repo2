#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#define NOMINMAX
#define CL_TARGET_OPENCL_VERSION 120
#include <Windows.h>
#include <CL/cl.h>
#include <opencv2/highgui.hpp>
using namespace cv;
using std::vector;

static char const* julia_kernel_src = R"~(
__kernel void julia_32f(
	__global float const* src, __global float* dst,
	int rows, int cols, int iter, double orgX, double orgY, double ppi)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x >= cols || y >= rows)
		return;

	double cx = orgX + (x - cols / 2) * ppi;
	double cy = orgY + (y - rows / 2) * ppi;
	double zx = 0.0, zy = 0.0;
	int i = 0;
	for (; i < iter; ++i)
	{
		double rx = zx * zx;
		double ry = zy * zy;
		if (rx + ry > 4.0)
			break;
		zy = cy + zx * zy * 2.0;
		zx = cx + rx - ry;
	}
	
	int srcidx = 3 * (int)(255.F * i / iter + 0.5F);
	int dstidx = 3 * mad24(y, cols, x);
	dst[dstidx + 0] = src[srcidx + 0];
	dst[dstidx + 1] = src[srcidx + 1];
	dst[dstidx + 2] = src[srcidx + 2];
}
)~";


struct Julia
{
	enum
	{
		ndiv = 16,
		size = 540,
		iter = 255,
	};

	cl_platform_id plat = NULL;
	cl_device_id dev = NULL;
	cl_uint ndev;
	cl_uint nplat;
	cl_int err;
	cl_context ctx;
	cl_command_queue cmdq;

	cl_mem cl_dst, cl_src;
	cl_program prog;
	cl_kernel kernel;
	char buffer[1024];

	Mat image, color;
	double ppi;
	Complexd org;
	vector<Vec3f> colormap;

	void checkError(char const* info)
	{
		if (err != CL_SUCCESS)
		{
			fprintf(stderr, "OpenCL Error %d when %s\n", err, info);
			exit(10);
		}
	}

	Julia()
	{
		image.create(size, size, CV_32FC3);
		org.re = org.im = 0;
		ppi = 4.0 / size;

		color.create(1, 256, CV_32FC3);
		Vec3f* ptr = color.ptr<Vec3f>();
		for (int i = 0; i < 256; ++i)
		{
			float n = 4.F * i / 256.F;
			float r = min(max(min(n - 1.5F, -n + 4.5F), 0.F), 1.F);
			float g = min(max(min(n - 0.5F, -n + 3.5F), 0.F), 1.F);
			float b = min(max(min(n + 0.5F, -n + 2.5F), 0.F), 1.F);
			ptr[255 - i] = Vec3f(b, g, r);
		}

		err = clGetPlatformIDs(1, &plat, &nplat);
		checkError("clGetPlatformIDs");

		err = clGetDeviceIDs(plat, CL_DEVICE_TYPE_GPU, 1, &dev, &ndev);
		checkError("clGetDeviceIDs");

		ctx = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
		checkError("clCreateContext");

		cmdq = clCreateCommandQueue(ctx, dev, NULL, &err);
		checkError("clCreateCommandQueue");

		err = clGetDeviceInfo(dev, CL_DEVICE_NAME, sizeof(buffer) - 1, buffer, NULL);
		printf("OpenCL %d %s\n", nplat, buffer);

		cl_dst = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, image.step[0] * size, NULL, &err);
		checkError("clCreateBuffer cl_dst");

		cl_src = clCreateBuffer(ctx, CL_MEM_READ_ONLY, color.step[0], NULL, &err);
		checkError("clCreateBuffer cl_src");

		err = clEnqueueWriteBuffer(cmdq, cl_src, CL_TRUE, 0, color.step[0], color.data, 0, NULL, NULL);
		checkError("clEnqueueWriteBuffer");

		prog = clCreateProgramWithSource(ctx, 1, &julia_kernel_src, NULL, &err);
		checkError("clCreateProgramWithSource");
		err = clBuildProgram(prog, 1, &dev, NULL, NULL, NULL);
		if (err == CL_BUILD_PROGRAM_FAILURE)
		{
			clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG, sizeof(buffer) - 1, buffer, NULL);
			fputs(buffer, stderr);
		}
		checkError("clBuildProgram");

		kernel = clCreateKernel(prog, "julia_32f", &err);
		checkError("clCreateKernel");
	}

	~Julia()
	{
		clFlush(cmdq);
		clFinish(cmdq);
		clReleaseKernel(kernel);
		clReleaseProgram(prog);
		clReleaseMemObject(cl_src);
		clReleaseMemObject(cl_dst);
		clReleaseCommandQueue(cmdq);
		clReleaseContext(ctx);
	}

	void run()
	{
		size_t lsize = 8;
		size_t gsize = (size + lsize - 1) / lsize * lsize;
		cl_int cl_size = size, cl_iter = iter;
		size_t global_size[] = { gsize, gsize };
		size_t local_size[] = { lsize, lsize };
		err = CL_SUCCESS;
		err |= clSetKernelArg(kernel, 0, sizeof(void*), &cl_src);
		err |= clSetKernelArg(kernel, 1, sizeof(void*), &cl_dst);
		err |= clSetKernelArg(kernel, 2, sizeof(cl_int), &cl_size);
		err |= clSetKernelArg(kernel, 3, sizeof(cl_int), &cl_size);
		err |= clSetKernelArg(kernel, 4, sizeof(cl_int), &cl_iter);
		err |= clSetKernelArg(kernel, 5, sizeof(double), &org.re);
		err |= clSetKernelArg(kernel, 6, sizeof(double), &org.im);
		err |= clSetKernelArg(kernel, 7, sizeof(double), &ppi);
		checkError("clSetKernelArg");

		err |= clEnqueueNDRangeKernel(cmdq, kernel, 2, NULL, global_size, local_size, 0, NULL, NULL);
		checkError("clEnqueueNDRangeKernel");

		clEnqueueReadBuffer(cmdq, cl_dst, CL_TRUE, 0, image.step[0] * size, image.data, 0, NULL, NULL);
		checkError("clEnqueueReadBuffer");
	}
};



int main()
{
	Julia ju;
	Complexd jmd(0.27322626, 0.595153338);
	double radius = 2.0, ratio = 0.98;
	char const* wd = "Julia";
	int64_t ticksum = 0;
	int frame = 0;
	namedWindow(wd);
	while (tolower(waitKey(1)) != 'q')
	{
		int64_t tick0 = getTickCount();
		radius *= ratio;
		if ((radius <= 1e-7) || (radius >= 2.0F))
			ratio = 1.0 / ratio;
		ju.org = jmd;
		ju.ppi = radius * 2.0 / ju.size;
		ju.run();
		imshow(wd, ju.image);
		int64_t tick1 = getTickCount();
		ticksum += tick1 - tick0;
		if (((++frame) & 31) == 0)
		{
			double dt = ticksum / getTickFrequency();
			printf("radius = %.9f, frame %4d, %8.3fms, ~ %.2ffps\n",
				radius, frame, 1e3 * dt, 32.0 / dt);
			ticksum = 0;
		}
	}
}
