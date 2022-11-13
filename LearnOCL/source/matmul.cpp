#define _CRT_SECURE_NO_WARNINGS
#include <cmath>
#include "base.hpp"

static int const TS = 8;
static int const WS = 4;

class OCL
{
	cl_platform_id platform;
	cl_device_id device;
	cl_context context;
	cl_command_queue cqueue;
	cl_program program;
	// H, W, C
	int nkernel;

public:
	OCL();
	~OCL();

	void init_ocl();
	void init_prog();
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

void OCL::init_ocl()
{
	cl_platform_id pid[4];
	cl_uint nid;
	size_t ngot;
	char name[64], profile[64], vendor[64], version[64], ext[4096];
	clGetPlatformIDs(4, pid, &nid);
	for (cl_uint i = 10; i < nid; ++i)
	{
		clGetPlatformInfo(pid[i], CL_PLATFORM_NAME, sizeof(name), name, &ngot);
		clGetPlatformInfo(pid[i], CL_PLATFORM_PROFILE, sizeof(profile), profile, &ngot);
		clGetPlatformInfo(pid[i], CL_PLATFORM_VENDOR, sizeof(vendor), vendor, &ngot);
		clGetPlatformInfo(pid[i], CL_PLATFORM_VERSION, sizeof(version), version, &ngot);
		clGetPlatformInfo(pid[i], CL_PLATFORM_EXTENSIONS, sizeof(ext), ext, &ngot);
		fprintf(stderr,
			"\nPlatform %u\n"
			"CL_PLATFORM_NAME       : %s\n"
			"CL_PLATFORM_PROFILE    : %s\n"
			"CL_PLATFORM_VENDOR     : %s\n"
			"CL_PLATFORM_VERSION    : %s\n"
			"CL_PLATFORM_EXTENSIONS : \n",
			i, name, profile, vendor, version);
		// fprintf(stderr, "%s\n", ext);
		for (size_t p = 0, k = 1; k <= ngot; ++k)
		{
			if (ext[k] != ' ' && k < ngot)
				continue;
			strcpy(name, "\t");
			strncat(name, ext + p, k - p);
			strncat(name, "\n", 1);
			fputs(name, stderr);
			p = k + 1;
		}
	}
	platform = pid[0];
	fprintf(stderr, "selelct platform 1 / %u\n", nid);

	cl_device_id did[4];
	clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 4, did, &nid);
	for (cl_uint i = 10; i < nid; ++i)
	{
		clGetDeviceInfo(did[i], CL_DEVICE_NAME, sizeof(name), name, &ngot);
		clGetDeviceInfo(did[i], CL_DEVICE_PROFILE, sizeof(profile), profile, &ngot);
		clGetDeviceInfo(did[i], CL_DEVICE_VENDOR, sizeof(vendor), vendor, &ngot);
		clGetDeviceInfo(did[i], CL_DEVICE_VERSION, sizeof(version), version, &ngot);
		clGetDeviceInfo(did[i], CL_DEVICE_EXTENSIONS, sizeof(ext), ext, &ngot);
		fprintf(stderr,
			"\nDevice %u\n"
			"CL_DEVICE_NAME       : %s\n"
			"CL_DEVICE_PROFILE    : %s\n"
			"CL_DEVICE_VENDOR     : %s\n"
			"CL_DEVICE_VERSION    : %s\n"
			"CL_DEVICE_EXTENSIONS : \n",
			i, name, profile, vendor, version);
		for (size_t p = 0, k = 1; k <= ngot; ++k)
		{
			if (ext[k] != ' ' && k < ngot)
				continue;
			string e;
			strcpy(name, "\t");
			strncat(name, ext + p, k - p);
			strncat(name, "\n", 1);
			fputs(name, stderr);
			p = k + 1;
		}
	}
	device = did[0];
	fprintf(stderr, "select device 1 / %u\n\n", nid);

	cl_int err;
	cl_context_properties prop[] = {
		CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platform),
		0, 0};
	CheckCLError(context = clCreateContext(prop, 1, &device, NULL, NULL, &err));
	CheckCLError(cqueue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
}

void OCL::init_prog()
{
	cl_int err;
	char info[4096];
	string K = string(__FILE__);
	K = K.substr(0, K.size() - 4) + ".cl";
	K = loadCLFile(K.data());
	char const* KS[] = {K.data()};
	snprintf(info, sizeof(info), "-cl-std=CL2.0 -cl-kernel-arg-info -Werror -DTS=%d -DWS=%d", TS, WS);
	CheckCLError(program = clCreateProgramWithSource(context, 1, KS, 0, &err));
	err = clBuildProgram(program, 1, &device, info, NULL, NULL);
	clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(info), info, NULL);
	if (err)
	{
		fprintf(stderr, "build program FAILED, code %d, error:\n%s", err, info);
		return;
	}
	fprintf(stderr, "build program end with code %d, log:\n%s", err, info);
	CheckCLError(err = clGetProgramInfo(program, CL_PROGRAM_KERNEL_NAMES, sizeof(info), info, NULL));
	fprintf(stderr, "kernel names: %s\n", info);
	nkernel = info[0] != 0;
	for (size_t i = 1; i < _countof(info) && info[i]; ++i)
		nkernel += info[i] == ';';
}

void OCL::work()
{
	cl_int err;
	cl_event e;
	cl_int const M = 4096, N = 5120, Q = 4096;
	Vec4z szlocal(TS, TS), sztotal(M, Q);
	Mat A(M, N, CV_32F), B(N, Q, CV_32F), C(M, Q, CV_32F), D(M, Q, CV_32F);
	randu(A, -8.0, nextafter(8.0, 9.0));
	randu(B, -8.0, nextafter(8.0, 9.0));
	CheckCLError(cl_mem a = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR + CL_MEM_READ_ONLY, A.total() * A.elemSize(), A.data, &err));
	CheckCLError(cl_mem b = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR + CL_MEM_READ_ONLY, B.total() * B.elemSize(), B.data, &err));
	CheckCLError(cl_mem c = clCreateBuffer(context, CL_MEM_WRITE_ONLY, D.total() * D.elemSize(), NULL, &err));
	clFlush(cqueue), clFinish(cqueue);
	fprintf(stderr, "upload matrix done\n");

	char KS[32];
	for (int i = 0; i < nkernel; ++i)
	{
		swap(C, D);
		snprintf(KS, sizeof(KS), "matmul%d", i);
		CheckCLError(cl_kernel K = clCreateKernel(program, KS, &err));
		CheckCLError(err = clSetKernelArg(K, 0, sizeof(M), &M));
		CheckCLError(err = clSetKernelArg(K, 1, sizeof(M), &N));
		CheckCLError(err = clSetKernelArg(K, 2, sizeof(M), &Q));
		CheckCLError(err = clSetKernelArg(K, 3, sizeof(a), &a));
		CheckCLError(err = clSetKernelArg(K, 4, sizeof(b), &b));
		CheckCLError(err = clSetKernelArg(K, 5, sizeof(c), &c));
		if (i >= 2)
			sztotal = Vec4z(M, (N + WS - 1) / WS);
		makeDiv(sztotal, szlocal);
		CheckCLError(err = clEnqueueNDRangeKernel(cqueue, K, 2, NULL, sztotal.val, szlocal.val, 0, NULL, &e));
		CheckCLError(clEnqueueReadBuffer(cqueue, c, CL_TRUE, 0, D.total() * D.elemSize(), D.data, 1, &e, NULL));
		clFlush(cqueue), clFinish(cqueue);
		CheckCLError(err = clWaitForEvents(1, &e));
		getCLTime(e, KS);
		CheckCLError(err = clReleaseKernel(K));
	}
	CheckCLError(err = clReleaseMemObject(a));
	CheckCLError(err = clReleaseMemObject(b));
	CheckCLError(err = clReleaseMemObject(c));
	CheckCLError(err = clReleaseEvent(e));

	absdiff(C, D, C);
	double dif = sum(C)[0];
	fprintf(stderr, "dif(C, D) = %f\n", dif);
}

int main()
{
	OCL ocl;
	ocl.init_ocl();
	ocl.init_prog();
	ocl.work();
	fputs("Game Over!\n", stderr);
}
