#include "../param/oclwrap.hpp"

#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_TARGET_OPENCL_VERSION  120
// #include <CL/cl2.hpp>

static void CL_CALLBACK clCallback(
	const char* errinfo,
	const void* private_info,
	size_t      cb,
	void*       user_data)
{
	puts(errinfo);
	(void)(private_info);
	(void)(cb);
	(void)(user_data);
}


int main()
{
	vector<Platform> plat;
	Platform::get(plat);
	vector<Device> device;
	Device::get(plat[0], device);

	cl_int err = 0;
	cl_context context = clCreateContext(NULL,
		1, &(device[0].handle), clCallback, NULL, &err);
	if (err != CL_SUCCESS) context = NULL;

	cl_command_queue cmdq = clCreateCommandQueue(context, device[0].handle, NULL, &err);
	if (err != CL_SUCCESS)
		cmdq = NULL;

	clReleaseCommandQueue(cmdq);
	clReleaseContext(context);

}

