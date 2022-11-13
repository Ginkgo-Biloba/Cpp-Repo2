#define _CRT_SECURE_NO_WARNINGS
#include <cmath>
#include <opencv2/imgproc.hpp>
#include "base.hpp"

static int const HistBins = 256;

static void getGaussianKernel(Mat& kernel)
{
	float sum = 0;
	int ksize = 37;
	int radius = ksize / 2;
	kernel.create(ksize, ksize, CV_32FC1);
	for (int h = -radius; h <= radius; ++h)
		for (int w = -radius; w <= radius; ++w)
		{
			int x = abs(h * 2 - w * 3);
			int y = abs(h * 2 + w * 3);
			float val = expf(-0.01 * x) * expf(-0.1 * y);
			sum += val;
			kernel.at<float>(h + radius, w + radius) = val;
		}
	for (int h = 0; h < ksize; ++h)
		for (int w = 0; w < ksize; ++w)
			kernel.at<float>(h, w) /= sum;
}

class OCL
{
	cl_platform_id platform;
	cl_device_id device;
	cl_context context;
	cl_command_queue cqueue;
	cl_program program;
	cl_uint cunits;
	size_t cwgs;

public:
	OCL();
	~OCL();

	void init();
	void work();
};

OCL::OCL()
{
	memset(this, 0, sizeof(*this));
}

OCL::~OCL()
{
	if (program) clReleaseProgram(program);
	if (cqueue) clReleaseCommandQueue(cqueue);
	if (context) clReleaseContext(context);
	if (device) clReleaseDevice(device);
}

void OCL::init()
{
	cl_int err;
	CheckCLError(err = clGetPlatformIDs(1, &platform, NULL));
	CheckCLError(err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL));
	cl_context_properties prop[] = {
		CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platform),
		0, 0};
	CheckCLError(context = clCreateContext(prop, 1, &device, NULL, NULL, &err));
	CheckCLError(cqueue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
	CheckCLError(err = clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cunits), &cunits, NULL));
	CheckCLError(err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(cwgs), &cwgs, NULL));
	char info[4096] = {0};
	string K = string(__FILE__);
	K = K.substr(0, K.size() - 4) + ".cl";
	K = loadCLFile(K.data());
	char const* KS[] = {K.data()};
	snprintf(info, sizeof(info), "-cl-kernel-arg-info -Werror -DHistBins=%d", HistBins);
	CheckCLError(program = clCreateProgramWithSource(context, 1, KS, 0, &err));
	err = clBuildProgram(program, 1, &device, info, NULL, NULL);
	clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(info), info, NULL);
	fprintf(stderr, "build program with code %d, log:\n%s", err, info);
	CheckCLError((void)(err));
	CheckCLError(err = clGetProgramInfo(program, CL_PROGRAM_KERNEL_NAMES, sizeof(info), info, NULL));
	fprintf(stderr, "kernel names: %s\n", info);
}

void OCL::work()
{
	Mat src, dst, filter;
	getGaussianKernel(filter);
	assert(jpgRead("sample\\20200518_002047.jpg", dst));
	cvtColor(dst, src, cv::COLOR_RGB2RGBA);
	cl_int err = src.isContinuous();
	cl_event e1, e2;
	Vec4z szloc = Vec4z::all(cwgs), sztot = Vec4z::all(cwgs * cunits);
	int total = src.rows * src.cols * src.channels();

	/// 直方图
	int hist[HistBins], chist[HistBins];
	CheckCLError(cl_mem M1 = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR + CL_MEM_READ_ONLY, total, src.data, &err));
	CheckCLError(cl_mem M2 = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(chist), NULL, &err));
	CheckCLError(cl_kernel K1 = clCreateKernel(program, "histogram", &err));
	CheckCLError(err = clEnqueueFillBuffer(cqueue, M2, &err, sizeof(err), 0, sizeof(chist), 0, NULL, NULL));
	CheckCLError(err = clSetKernelArg(K1, 0, sizeof(M1), &M1));
	CheckCLError(err = clSetKernelArg(K1, 1, sizeof(int), &total));
	CheckCLError(err = clSetKernelArg(K1, 2, sizeof(M2), &M2));
	CheckCLError(err = clEnqueueNDRangeKernel(cqueue, K1, 1, NULL, sztot.val, szloc.val, 0, NULL, &e1));
	CheckCLError(clEnqueueReadBuffer(cqueue, M2, CL_TRUE, 0, sizeof(chist), chist, 1, &e1, NULL));
	clFlush(cqueue), clFinish(cqueue);
	clWaitForEvents(1, &e1);
	getCLTime(e1, "histogram");
	CheckCLError(err = clReleaseKernel(K1));
	CheckCLError(err = clReleaseMemObject(M2));
	CheckCLError(err = clReleaseMemObject(M1));
	int dif = 0;
	for (int i = 0; i < total; ++i)
		++(hist[src.data[i]]);
	for (int i = 0; i < HistBins; ++i)
		dif += abs(hist[i] - chist[i]);
	fprintf(stderr, "absdiff(cpu, ocl) = %d\n", dif);

	/// 旋转、卷积
	szloc = Vec4z(16, 8);
	sztot = Vec4z(src.cols, src.rows, 1);
	makeDiv(sztot, szloc);
	cl_image_format ifmt;
	cl_image_desc desc;
	desc.image_type = CL_MEM_OBJECT_IMAGE2D;
	desc.image_width = src.cols;
	desc.image_height = src.rows;
	desc.image_depth = 0;
	desc.image_array_size = 0;
	desc.image_row_pitch = 0;
	desc.image_slice_pitch = 0;
	desc.num_mip_levels = 0;
	desc.num_samples = 0;
	desc.buffer = NULL;
	ifmt.image_channel_order = CL_RGBA;
	ifmt.image_channel_data_type = CL_UNSIGNED_INT8;
	CheckCLError(cl_sampler S1 = clCreateSampler(context, CL_TRUE, CL_ADDRESS_CLAMP, CL_FILTER_LINEAR, &err));
	CheckCLError(cl_sampler S2 = clCreateSampler(context, CL_FALSE, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_NEAREST, &err));
	CheckCLError(cl_mem I1 = clCreateImage(context, CL_MEM_COPY_HOST_PTR + CL_MEM_READ_ONLY, &ifmt, &desc, src.data, &err));
	CheckCLError(cl_mem I2 = clCreateImage(context, CL_MEM_WRITE_ONLY, &ifmt, &desc, NULL, &err));
	CheckCLError(cl_mem I3 = clCreateImage(context, CL_MEM_WRITE_ONLY, &ifmt, &desc, NULL, &err));
	CheckCLError(cl_mem F1 = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR + CL_MEM_READ_ONLY, filter.total() * filter.elemSize(), filter.data, &err));
	CheckCLError(cl_kernel K2 = clCreateKernel(program, "rotation", &err));
	CheckCLError(cl_kernel K3 = clCreateKernel(program, "convolution", &err));
	CheckCLError(err = clSetKernelArg(K2, 0, sizeof(S1), &S1));
	CheckCLError(err = clSetKernelArg(K2, 1, sizeof(I1), &I1));
	CheckCLError(err = clSetKernelArg(K2, 2, sizeof(I2), &I2));
	CheckCLError(err = clSetKernelArg(K2, 3, sizeof(int), &src.rows));
	CheckCLError(err = clSetKernelArg(K2, 4, sizeof(int), &src.cols));
	CheckCLError(err = clSetKernelArg(K3, 0, sizeof(S2), &S2));
	CheckCLError(err = clSetKernelArg(K3, 1, sizeof(I1), &I1));
	CheckCLError(err = clSetKernelArg(K3, 2, sizeof(I3), &I3));
	CheckCLError(err = clSetKernelArg(K3, 3, sizeof(F1), &F1));
	CheckCLError(err = clSetKernelArg(K3, 4, sizeof(int), &src.rows));
	CheckCLError(err = clSetKernelArg(K3, 5, sizeof(int), &src.cols));
	CheckCLError(err = clSetKernelArg(K3, 6, sizeof(int), &filter.cols));
	CheckCLError(err = clEnqueueNDRangeKernel(cqueue, K2, 2, NULL, sztot.val, szloc.val, 0, NULL, &e1));
	CheckCLError(err = clEnqueueNDRangeKernel(cqueue, K3, 2, NULL, sztot.val, szloc.val, 0, NULL, &e2));
	clFlush(cqueue), clFinish(cqueue);
	CheckCLError(clWaitForEvents(1, &e1));
	CheckCLError(clWaitForEvents(1, &e2));
	getCLTime(e1, "rotation");
	getCLTime(e2, "convolution");
	szloc = Vec4z::all(0), sztot = Vec4z(src.cols, src.rows, 1);
	CheckCLError(err = clEnqueueReadImage(cqueue, I2, CL_TRUE, szloc.val, sztot.val, 0, 0, src.data, 0, NULL, NULL));
	cvtColor(src, dst, cv::COLOR_RGBA2RGB);
	jpgWrite("sample\\20200518_002047-1.jpg", dst);
	CheckCLError(err = clEnqueueReadImage(cqueue, I3, CL_TRUE, szloc.val, sztot.val, 0, 0, src.data, 0, NULL, NULL));
	cvtColor(src, dst, cv::COLOR_RGBA2RGB);
	jpgWrite("sample\\20200518_002047-2.jpg", dst);
	CheckCLError(clReleaseKernel(K3));
	CheckCLError(clReleaseKernel(K2));
	CheckCLError(clReleaseMemObject(F1));
	CheckCLError(clReleaseMemObject(I3));
	CheckCLError(clReleaseMemObject(I2));
	CheckCLError(clReleaseMemObject(I1));
	CheckCLError(clReleaseSampler(S2));
	CheckCLError(clReleaseSampler(S1));
	CheckCLError(clReleaseEvent(e2));
	CheckCLError(clReleaseEvent(e1));
}

int main()
{
	OCL ocl;
	ocl.init();
	ocl.work();
	fputs("Game Over!\n", stderr);
}
