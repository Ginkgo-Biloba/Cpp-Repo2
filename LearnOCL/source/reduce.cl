#ifndef WGS
#	define WGS 0
#endif

__kernel void reduce(__global uchar const* src, int const len, __global uint* dst)
{
	int const li = get_local_id(0);
	int const pi = get_group_id(0);
	int const group = get_num_groups(0);
	__local uint S[WGS];
	S[li] = 0;
	for (int i = get_global_id(0); i < len; i += get_global_size(0))
		S[li] += src[i];
	work_group_barrier(CLK_LOCAL_MEM_FENCE);
	for (int i = WGS >> 1; i > 0; i >>= 1)
	{
		if (li < i)
			S[li] += S[li + i];
		work_group_barrier(CLK_LOCAL_MEM_FENCE);
	}
	if (li == 0)
		dst[pi] = S[0];
}


