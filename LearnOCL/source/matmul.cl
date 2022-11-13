
#ifndef TS
#	define TS 8
#endif

#ifndef WS
#	define WS 8
#endif

__kernel void matmul0(int const M, int const N, int const Q,
	__global float const* A, __global float const* B, __global float* C)
{
	int gh = get_global_id(0);
	int gw = get_global_id(1);
	if (gh >= M || gw >= Q)
		return;
	float val = 0;
	for (int i = 0; i < N; ++i)
		val += A[gh * N + i] * B[i * Q + gw];
	C[gh * Q + gw] = val;
}

__kernel void matmul1(int const M, int const N, int const Q,
	__global float const* A, __global float const* B, __global float* C)
{
	int const lh = get_local_id(0);
	int const lw = get_local_id(1);
	int const gh = get_global_id(0); // get_group_id(0) * TS + lh;
	int const gw = get_global_id(1); // get_group_id(1) * TS + lw;
	int const tile = (N + TS - 1) / TS;
	__private float val = 0;
	__local float a[TS][TS], b[TS][TS];
	for (int t = 0; t < tile; ++t)
	{
		int const th = TS * t + lh;
		int const tw = TS * t + lw;
		a[lh][lw] = (gh < M && tw < N) ? A[gh * N + tw] : 0;
		b[lw][lh] = (th < N && gw < Q) ? B[th * Q + gw] : 0;
		work_group_barrier(CLK_LOCAL_MEM_FENCE);
		for (int i = 0; i < TS; ++i)
			val += a[lh][i] * b[lw][i];
		work_group_barrier(CLK_LOCAL_MEM_FENCE);
	}
	C[gh * Q + gw] = val;
}


__kernel void matmul2(int const M, int const N, int const Q,
	__global float const* A, __global float const* B, __global float* C)
{
	int const lh = get_local_id(0);
	int const lw = get_local_id(1);
	int const gh = get_global_id(0);
	int const gw = get_global_id(1) * WS;
	int const tile = (N + TS - 1) / TS;
	__private float c[WS];
	__local float a[TS][TS], b[TS][TS * WS];
	for (int i = 0; i < WS; ++i)
		c[i] = 0;
	for (int t = 0; t < tile; ++t)
	{
		int const th = TS * t + lh;
		int const tw = TS * t + lw;
		a[lh][lw] = (gh < M && tw < N) ? A[gh * N + tw] : 0;
		for (int p = 0; p < WS; ++p)
			b[lh][lw * WS + p] = (th < N && gw + p < Q) ? B[th * Q + gw + p] : 0;
		work_group_barrier(CLK_LOCAL_MEM_FENCE);
		for (int i = 0; i < TS; ++i)
			for (int p = 0; p < WS; ++p)
				c[p] += a[lh][i] * b[i][lw * WS + p];
		work_group_barrier(CLK_LOCAL_MEM_FENCE);
	}
	for (int p = 0; p < WS; ++p)
		if (gh < M && gw + p < Q)
			C[gh * Q + gw + p] = c[p];
}
