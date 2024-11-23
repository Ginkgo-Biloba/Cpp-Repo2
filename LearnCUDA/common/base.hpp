#include <cstdio>
#include <cuda_runtime.h>
#undef NDEBUG
#include <cassert>
#include <cstdlib>

void _check_error(cudaError_t err, int line, char const* file, char const* expr)
{
	if (err == cudaSuccess)
		return;
	char buf[4096];
	snprintf(buf, sizeof(buf),
		"error %d (%s) in file %s line %d when call `%s'\n:\t%s\n",
		static_cast<int>(err), cudaGetErrorName(err), file, line, expr, cudaGetErrorString(err));
}

inline bool _always_bool(bool b)
{
	return b;
}


#define CheckError(expr)                          \
	do {                                            \
		cudaError_t err = (expr);                     \
		_check_error(err, __LINE__, __FILE__, #expr); \
	} while (_always_bool(false));


int getCorePerSP(cudaDeviceProp const& prop)
{
#define ComputeCapacity(a, b) ((a << 8) + b)
#define ComputeCase(a, b, x) \
	case ComputeCapacity(a, b): return x;
	switch (ComputeCapacity(prop.major, prop.minor))
	{
		// Fermi
		ComputeCase(2, 0, 48);
		ComputeCase(2, 1, 48);
		// Kepler
		ComputeCase(3, 0, 192);
		ComputeCase(3, 5, 192);
		ComputeCase(3, 7, 192);
		// Maxwell
		ComputeCase(5, 0, 128);
		ComputeCase(5, 2, 128);
		// Pascal
		ComputeCase(6, 0, 64);
		ComputeCase(6, 1, 128);
		// Volta and Turing
		ComputeCase(7, 0, 64);
		ComputeCase(7, 5, 64);
		// Ampere
		ComputeCase(8, 0, 64);
		ComputeCase(8, 6, 128);
		ComputeCase(8, 9, 128);
		// Hopper
		ComputeCase(9, 0, 128);
	default:
		fprintf(stderr, "unknow compute capability %d.%d", prop.major, prop.minor);
	}
#undef ComputeCapacity
#undef ComputeCase
	return -1;
}
