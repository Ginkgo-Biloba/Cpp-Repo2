
#ifndef TS
#	define TS 16
#endif

#ifndef WS
#	define WS 4
#endif

__kernel void matt0(int const M, int const N, __global float const* A, __global float* B)
{
	int const w = get_global_id(0);
	int const h = get_global_id(1);
	if (h < M && w < N)
		B[mad24(w, M, h)] = A[mad24(h, N, w)];
}

__kernel void matt1(int const M, int const N, __global float const* A, __global float* B)
{
	int const lw = get_local_id(0);
	int const lh = get_local_id(1);
	int const pw = get_group_id(0) * TS;
	int const ph = get_group_id(1) * TS;
	__local float lbuf[TS][TS + 1];
	int h = ph + lh;
	int w = pw + lw;
	if (h < M && w < N)
		lbuf[lh][lw] = A[mad24(h, N, w)];
	work_group_barrier(CLK_LOCAL_MEM_FENCE);
	h = pw + lh;
	w = ph + lw;
	if (h < N && w < M)
		B[mad24(h, M, w)] = lbuf[lw][lh];
}


__kernel void matt2(int const M, int const N, __global float const* A, __global float* B)
{
	int const lw = get_local_id(0);
	int const lh = get_local_id(1);
	int const pw = get_group_id(0) * TS * WS;
	int const ph = get_group_id(1) * TS;
	__local float lbuf[TS][TS * WS + 1];
	int h = ph + lh;
	int w = pw + lw;
	for (int i = 0; i < TS * WS; i += TS)
	{
		if (h < M && w + i < N)
			lbuf[lh][lw + i] = A[mad24(h, N, w + i)];
	}
	work_group_barrier(CLK_LOCAL_MEM_FENCE);
	h = pw + lh;
	w = ph + lw;
	for (int i = 0; i < TS * WS; i += TS)
	{
		if (h + i < N && w < M)
			B[mad24(h + i, M, w)] = lbuf[lw][lh + i];
	}
}
