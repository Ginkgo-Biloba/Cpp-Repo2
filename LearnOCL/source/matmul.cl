
#ifndef TS
#	define TS 8
#endif

#ifndef WS
#	define WS 8
#endif

__kernel void matmul0(int const M, int const N, int const Q,
	__global float const* A, __global float const* B, __global float* C)
{
	int w = get_global_id(0);
	int h = get_global_id(1);
	if (h >= M || w >= Q)
		return;
	float val = 0;
	for (int i = 0; i < N; ++i)
		val += A[h * N + i] * B[i * Q + w];
	C[h * Q + w] = val;
}

__kernel void matmul1(int const M, int const N, int const Q,
	__global float const* A, __global float const* B, __global float* C)
{
	int const lw = get_local_id(0);
	int const lh = get_local_id(1);
	int const gw = get_global_id(0);
	int const gh = get_global_id(1);
	float val = 0;
	__local float a[TS][TS], b[TS][TS];
	for (int t = 0; t < N; t += TS)
	{
		int const th = t + lh;
		int const tw = t + lw;
		a[lh][lw] = (gh < M && tw < N) ? A[mad24(gh, N, tw)] : 0;
		b[lh][lw] = (th < N && gw < Q) ? B[mad24(th, Q, gw)] : 0;
		work_group_barrier(CLK_LOCAL_MEM_FENCE);
		for (int i = 0; i < TS; ++i)
			val += a[lh][i] * b[i][lw];
		work_group_barrier(CLK_LOCAL_MEM_FENCE);
	}
	C[gh * Q + gw] = val;
}


__kernel void matmul2(int const M, int const N, int const Q,
	__global float const* A, __global float const* B, __global float* C)
{
	int const lw = get_local_id(0);
	int const lh = get_local_id(1);
	int const pw = get_group_id(0) * TS * WS;
	int const ph = get_group_id(1) * TS;
	float c[WS];
	__local float a[TS][TS], b[TS][TS * WS];
	for (int i = 0; i < WS; ++i)
		c[i] = 0;
	for (int t = 0; t < N; t += TS)
	{
		int h = ph + lh;
		int w = t + lw;
		a[lh][lw] = (h < M && w < N) ? A[mad24(h, N, w)] : 0;
		for (int p = 0; p < WS; ++p)
		{
			h = t + lh;
			w = pw + lw + TS * p;
			b[lh][w - pw] = (h < N && w < Q) ? B[mad24(h, Q, w)] : 0;
		}
		work_group_barrier(CLK_LOCAL_MEM_FENCE);
		for (int i = 0; i < TS; ++i)
			for (int p = 0; p < WS; ++p)
				c[p] += a[lh][i] * b[i][TS * p + lw];
		work_group_barrier(CLK_LOCAL_MEM_FENCE);
	}
	for (int p = 0; p < WS; ++p)
	{
		int h = ph + lh;
		int w = pw + lw + TS * p;
		if (h < M && w < Q)
			C[mad24(h, Q, w)] = c[p];
	}
}
