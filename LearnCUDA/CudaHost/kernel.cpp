
#include <opencv2/imgproc.hpp>
#include <cuda_runtime.h>
using namespace cv;
using std::string;

#if defined _MSC_VER
#	ifdef _DEBUG
__pragma(comment(lib, "opencv_core2413d.lib"));
__pragma(comment(lib, "opencv_imgproc2413d.lib"));
#	else
__pragma(comment(lib, "opencv_core2413.lib"));
__pragma(comment(lib, "opencv_imgproc2413.lib"));
#	endif
#elif defined __GNUC__
#else
#endif

int const warm = 10;
int const loop = 50;
int const M = 1280, K = 4096, N = 1024;
Mat mA, mB;
float *dA, *dB;
size_t const zA = M * K * sizeof(float);
size_t const zB = K * N * sizeof(float);
size_t const zC = M * N * sizeof(float);

static void sample()
{
	randu(mA, -100, +100);
	randn(mB, 0, 100);
	cudaMemcpy(dA, mA.data, zA, cudaMemcpyHostToDevice);
	cudaMemcpy(dB, mB.data, zB, cudaMemcpyHostToDevice);
	cudaDeviceSynchronize();
}

template <typename T>
static int64 work(T& t, int i)
{
	Mat A(M, K, CV_32F, t.A);
	Mat B(K, N, CV_32F, t.B);
	Mat C(M, N, CV_32F, t.C);
	int64 t1 = getTickCount();
	t.data();
	boxFilter(A, A, -1, Size(5, 5));
	boxFilter(B, B, -1, Size(5, 5));
	// C = A * B;
	int64 t2 = getTickCount();
	return i < warm ? 0 : t2 - t1;
}

template <typename T>
static double sweep(T& t)
{
	Mat A(M, K, CV_32F, t.A);
	Mat B(K, N, CV_32F, t.B);
	Mat C(M, N, CV_32F, t.C);
	Scalar s = sum(A) + sum(B) + sum(C);
	return s[0];
}

static void output(char const* func, int64 const* tick)
{
	double rq = 1e3 / getTickFrequency() / (loop - warm);
	fprintf(stderr,
		"%40s  %17.3f  %17.3f  %17.3f\n",
		func, tick[0] * rq, tick[1] * rq, tick[2] * rq);
}

struct TCpu
{
	float *A, *B, *C;
	void alloc()
	{
		A = static_cast<float*>(fastMalloc(zA));
		B = static_cast<float*>(fastMalloc(zB));
		C = static_cast<float*>(fastMalloc(zC));
	}
	void free()
	{
		fastFree(A);
		fastFree(B);
		fastFree(C);
	}
	void data()
	{
		memcpy(A, mA.data, zA);
		memcpy(B, mB.data, zB);
		A[0] = B[1024] = 1.f;
	}
};

struct THost
{
	float *A, *B, *C;
	void alloc()
	{
		cudaMallocHost(reinterpret_cast<void**>(&A), zA);
		cudaMallocHost(reinterpret_cast<void**>(&B), zB);
		cudaMallocHost(reinterpret_cast<void**>(&C), zC);
	}
	void free()
	{
		cudaFreeHost(A);
		cudaFreeHost(B);
		cudaFreeHost(C);
	}
	void data()
	{
		cudaMemcpyAsync(A, dA, zA, cudaMemcpyDeviceToHost);
		cudaMemcpyAsync(B, dB, zB, cudaMemcpyDeviceToHost);
		cudaDeviceSynchronize();
		A[0] = B[1024] = 1.f;
	}
};

struct TManaged
{
	float *A, *B, *C;
	void alloc()
	{
		cudaMallocManaged(reinterpret_cast<void**>(&A), zA);
		cudaMallocManaged(reinterpret_cast<void**>(&B), zB);
		cudaMallocManaged(reinterpret_cast<void**>(&C), zC);
	}
	void free()
	{
		cudaFree(A);
		cudaFree(B);
		cudaFree(C);
	}
	void data()
	{
		cudaMemcpyAsync(A, dA, zA, cudaMemcpyDeviceToDevice);
		cudaMemcpyAsync(B, dB, zB, cudaMemcpyDeviceToDevice);
		cudaMemPrefetchAsync(A, zA, cudaCpuDeviceId);
		cudaMemPrefetchAsync(B, zB, cudaCpuDeviceId);
		cudaDeviceSynchronize();
		A[0] = B[1024] = 1.f;
	}
};

static void timeit1(TCpu& tC, THost& tH, TManaged& tM)
{
	int64 tick[3] = {0, 0, 0};
	for (int i = 0; i < loop; ++i)
	{
		fprintf(stderr, "%s : %d\r", __FUNCTION__, i + 1);
		tC.alloc();
		tH.alloc();
		tM.alloc();
		sample();
		tick[0] += work(tC, i);
		tick[1] += work(tH, i);
		tick[2] += work(tM, i);
		tC.free();
		tH.free();
		tM.free();
	}
	output("N × [alloc + work + free]", tick);
}

static void timeit2(TCpu& tC, THost& tH, TManaged& tM)
{
	int64 tick[3] = {0, 0, 0};
	tC.alloc();
	tH.alloc();
	tM.alloc();
	for (int i = 0; i < loop; ++i)
	{
		fprintf(stderr, "%s : %d\r", __FUNCTION__, i + 1);
		sample();
		tick[0] += work(tC, i);
		tick[1] += work(tH, i);
		tick[2] += work(tM, i);
	}
	tC.free();
	tH.free();
	tM.free();
	output("alloc + N × [work] + free", tick);
}

static void timeit3(TCpu& tC, THost& tH, TManaged& tM)
{
	int64 tick[3] = {0, 0, 0};
	for (int i = 0; i < loop; ++i)
	{
		fprintf(stderr, "%s : %d\r", __FUNCTION__, i + 1);
		tC.alloc();
		tH.alloc();
		tM.alloc();
		sample();
		sweep(tC);
		sweep(tH);
		sweep(tM);
		tick[0] += work(tC, i);
		tick[1] += work(tH, i);
		tick[2] += work(tM, i);
		tC.free();
		tH.free();
		tM.free();
	}
	output("N × [alloc + sweep + work + free]", tick);
}

int main()
{
	cudaSetDevice(0);
	mA.create(M, K, CV_32F);
	mB.create(K, N, CV_32F);
	cudaMalloc(reinterpret_cast<void**>(&dA), zA);
	cudaMalloc(reinterpret_cast<void**>(&dB), zB);

	string line;
	for (int i = 0; i < 100; ++i)
		line += "─";
	line += "\n";
	static char const* name[] = {
		"fastMalloc",
		"cudaMallocHost",
		"cudaMallocManaged",
	};
	char info[256];

	TCpu tC;
	THost tH;
	TManaged tM;
	for (int i = 1; i <= 3; ++i)
	{
		snprintf(info, sizeof(info),
			"********** pass %d **********", i);
		fputs("\n", stderr);
		fputs(line.data(), stderr);
		fprintf(stderr,
			"%40s  %17s  %17s  %17s\n",
			info, name[0], name[1], name[2]);
		fputs(line.data(), stderr);
		timeit1(tC, tH, tM);
		timeit2(tC, tH, tM);
		timeit3(tC, tH, tM);
		fputs(line.data(), stderr);
	}
}
