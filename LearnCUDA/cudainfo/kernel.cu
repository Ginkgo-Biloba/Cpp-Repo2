
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include "../common/base.hpp"


int main()
{
	int count;
	CheckError(cudaGetDeviceCount(&count));
	printf("cuda count: %d\n", count);

	cudaDeviceProp prop;
	CheckError(cudaGetDeviceProperties(&prop, 0));
	printf("cuda core number per sp: %d\n", getCorePerSP(prop));

	int driver;
	CheckError(cudaDriverGetVersion(&driver));
	printf("cuda driver version: %d.%d\n",
		driver / 1000, (driver % 1000) / 10);

	int runtime;
	CheckError(cudaRuntimeGetVersion(&runtime));
	printf("cuda rumtime version: %d.%d\n",
		runtime / 1000, (runtime % 1000) / 10);

	puts("hello cuda");
}
