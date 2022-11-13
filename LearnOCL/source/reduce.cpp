#define _CRT_SECURE_NO_WARNINGS
#include <cmath>
#include "base.hpp"

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
	snprintf(info, sizeof(info), "-cl-kernel-arg-info -Werror -DWGS=%zd", cwgs);
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
	/// 求和
	cl_int err;
	cl_event e1, e2;
	AutoBuffer<int> S(cunits);
	Mat src(4096, 4096, CV_8U), dst(src.rows, src.cols, CV_32S);
	int total = static_cast<int>(src.total());
	Vec4z szloc = Vec4z::all(cwgs), sztot = Vec4z::all(cwgs * cunits);
	randu(src, 0, 127);
	CheckCLError(cl_mem M1 = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR + CL_MEM_READ_ONLY, total * src.elemSize(), src.data, &err));
	CheckCLError(cl_mem M2 = clCreateBuffer(context, CL_MEM_WRITE_ONLY, cunits * sizeof(S[0]), NULL, &err));
	CheckCLError(cl_kernel K1 = clCreateKernel(program, "reduce", &err));
	CheckCLError(clSetKernelArg(K1, 0, sizeof(M1), &M1));
	CheckCLError(clSetKernelArg(K1, 1, sizeof(int), &total));
	CheckCLError(clSetKernelArg(K1, 2, sizeof(M2), &M2));
	CheckCLError(clEnqueueNDRangeKernel(cqueue, K1, 1, NULL, sztot.val, szloc.val, 0, NULL, &e1));
	CheckCLError(clEnqueueReadBuffer(cqueue, M2, CL_FALSE, 0, cunits * sizeof(S[0]), S, 1, &e1, &e2));
	clFlush(cqueue), clFinish(cqueue);
	CheckCLError(clWaitForEvents(1, &e1));
	CheckCLError(clWaitForEvents(1, &e2));
	getCLTime(e1, "reduce");
	for (cl_uint i = 1; i < cunits; ++i)
		S[0] += S[i];
	for (int i = 0; i < total; ++i)
		S[0] -= src.data[i];
	fprintf(stderr, "diff(cpu, ocl) = %u\n", S[0]);
	CheckCLError(clReleaseKernel(K1));
	CheckCLError(clReleaseMemObject(M2));
	CheckCLError(clReleaseMemObject(M1));
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
