#include <cstdio>
#include <CL/cl.h>


int main()
{
	cl_uint nplat;

	cl_int err = clGetPlatformIDs(0, NULL, &nplat);
	if (err == CL_SUCCESS)
		printf("detected OpenCL platform: %d\n", nplat);
	else
		printf("error calling clGetPlatformIDs: %d\n", nplat);
	
}
