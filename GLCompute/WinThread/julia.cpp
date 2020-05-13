#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#define NOMINMAX
#include <Windows.h>
#include <opencv2/highgui.hpp>
using namespace cv;
using std::vector;

int ijulia = 1;

inline int atomic_load(int* ptr)
{
	return _InterlockedOr(
		reinterpret_cast<long volatile*>(ptr), 0);
}

inline int atomic_fetch_add(int* ptr, int val)
{
	return _InterlockedExchangeAdd(
		reinterpret_cast<long volatile*>(ptr), val);
}

struct Julia
{
	enum
	{
		ndiv = 16,
		size = 540,
		iter = 256,
	};

	Mat image;
	int current;
	float radius, ppi;
	Complexf org;
	Complexf c;
	vector<Vec3f> colormap;

	Julia()
	{
		image.create(size, size, CV_32FC3);
		current = 0;
		org.re = org.im = 0;
		c = Complexf(-1.F, -0.F);
		radius = 1.5F;
		ppi = radius * 2.F / size;

		colormap.resize(256);
		for (int i = 0; i < 256; ++i)
		{
			float n = 4.F * i / 256;
			float r = min(max(min(n - 1.5F, -n + 4.5F), 0.F), 1.F);
			float g = min(max(min(n - 0.5F, -n + 3.5F), 0.F), 1.F);
			float b = min(max(min(n + 0.5F, -n + 2.5F), 0.F), 1.F);
			colormap[255 - i] = Vec3f(b, g, r);
		}
	}


	void do_julia(int h, int hend)
	{
		for (; h < hend; ++h)
		{
			Vec3f* ptr = image.ptr<Vec3f>(h);
			float im = org.im + (h - size / 2) * ppi;
			for (int w = 0; w < size; ++w)
			{
				Complexf z(org.re + (w - size / 2) * ppi, im);
				int i = 0;
				do
				{
					if (z.re * z.re + z.im * z.im > 4.F)
						break;
					z = z * z + c;
					++i;
				} while (i < iter);
				ptr[w] = colormap[i];
			}
		}
	}

	void mandelbrot(int h, int hend)
	{
		for (; h < hend; ++h)
		{
			Vec3f* ptr = image.ptr<Vec3f>(h);
			float im = org.im + (h - size / 2) * ppi;
			for (int w = 0; w < size; ++w)
			{
				Complexf zc(org.re + (w - size / 2) * ppi, im);
				Complexf z;
				int i = 0;
				for (; i < iter; ++i)
				{
					float x = z.re * z.re;
					float y = z.im * z.im;
					if (x + y > 1024.F)
						break;
					z.im = z.re * z.im * 2.F + zc.im;
					z.re = x - y + zc.re;
				}
				ptr[w] = colormap[i];
			}
		}
	}

	int run()
	{
		int sumt = 0, task, hbeg, hend;
		for (;;)
		{
			task = atomic_load(&current);
			task = max((size - task) / ndiv, 1);
			hbeg = atomic_fetch_add(&current, task);
			if (hbeg >= size)
				break;
			hend = min(hbeg + task, static_cast<int>(size));
			sumt += (hend - hbeg);
			if (ijulia)
				do_julia(hbeg, hend);
			else
				mandelbrot(hbeg, hend);
		}
		return sumt;
	}
};


static VOID CALLBACK Julia_Func(PTP_CALLBACK_INSTANCE, PVOID vp_job, PTP_WORK)
{
	Julia* job = static_cast<Julia*>(vp_job);
	if (atomic_load(&(job->current)) < job->size)
		job->run();
}


int main()
{
	TP_POOL* pool = CreateThreadpool(NULL);
	if (pool == NULL)
	{
		printf("CreateThreadpool LastError: %u\n", GetLastError());
		return 1;
	}

	TP_CALLBACK_ENVIRON env;
	InitializeThreadpoolEnvironment(&env);
	SetThreadpoolCallbackPool(&env, pool);

	TP_CLEANUP_GROUP* clup = CreateThreadpoolCleanupGroup();
	if (clup == NULL)
	{
		printf("CreateThreadpoolCleanupGroup LastError: %u\n", GetLastError());
		return 2;
	}
	SetThreadpoolCallbackCleanupGroup(&env, clup, NULL);

	SYSTEM_INFO sys;
	GetSystemInfo(&sys);
	SetThreadpoolThreadMinimum(pool, 1);
	SetThreadpoolThreadMaximum(pool, sys.dwNumberOfProcessors * 2);

	Julia ju;
	Complexf juc(-0.8F, 0.156F);
	Complexf jmd(0.27322626F, 0.595153338F);
	float cdir = -0.002F, coff = 0.F;
	float rad = 2.F, ratio = 0.95F;
	char const* wd = "Julia";
	int64_t ticksum = 0;
	int step = 0;
	namedWindow(wd);
	while (tolower(waitKey(20)) != 'q')
	{
		ju.current = 0;
		if (ijulia)
		{
			ju.c = juc + coff;
			coff += cdir;
			if (fabs(coff) > 0.1F)
				cdir = -cdir;
		}
		else
		{
			rad *= ratio;
			if ((rad < FLT_EPSILON) || (rad > 2.1F))
				ratio = 1.F / ratio;
			ju.org = jmd;
			ju.ppi = rad * 2.F / ju.size;
		}

		int64_t tick0 = getTickCount();
		TP_WORK* work = CreateThreadpoolWork(Julia_Func, &ju, &env);
		if (!work)
		{
			printf("CreateThreadpoolWork LastError %u\n", GetLastError());
			break;
		}
		for (DWORD i = sys.dwNumberOfProcessors; i > 1; --i)
			SubmitThreadpoolWork(work);
		ju.run();
		WaitForThreadpoolWorkCallbacks(work, FALSE);
		CloseThreadpoolWork(work);
		int64_t tick1 = getTickCount();
		ticksum += tick1 - tick0;
		if (((++step) & 31) == 0)
		{
			if (ijulia)
				printf("c = (%f, %f)", ju.c.re, ju.c.im);
			else
				printf("rad = %.9f", rad);
			printf(", %fms\n", 1e3 * ticksum / getTickFrequency());
			ticksum = 0;
		}
		imshow(wd, ju.image);
	}

	CloseThreadpoolCleanupGroupMembers(clup, FALSE, NULL);
	CloseThreadpoolCleanupGroup(clup);
	DestroyThreadpoolEnvironment(&env);
	CloseThreadpool(pool);
}
