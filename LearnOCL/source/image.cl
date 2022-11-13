
#ifndef HistBins
#	define HistBins 0
#endif

__constant float theta = 10 * M_PI_F / 180;

__kernel void histogram(__global uchar const* src, int const len, __global int* dst)
{
	__local int L[HistBins];
	int li = get_local_id(0);
	int gi = get_global_id(0);
	for (int i = li; i < HistBins; i += get_local_size(0))
		L[i] = 0;
	work_group_barrier(CLK_LOCAL_MEM_FENCE);

	for (int i = gi; i < len; i += get_global_size(0))
		atomic_add(L + convert_int(src[i]), 1);
	work_group_barrier(CLK_LOCAL_MEM_FENCE);

	for (int i = li; i < HistBins; i += get_local_size(0))
		atomic_add(dst + i, L[i]);
}

__kernel void rotation(sampler_t const sampler, __read_only image2d_t src,
	__write_only image2d_t dst, int const rows, int const cols)
{
	int ix = get_global_id(0);
	int iy = get_global_id(1);
	if (ix >= cols || iy >= rows)
		return;
	float x = ix + 0.5 - cols * 0.5;
	float y = iy + 0.5 - rows * 0.5;
	float Ct, St = sincos(theta, &Ct);
	float2 pos;
	pos.x = (x * Ct - y * St) / cols + 0.5;
	pos.y = (x * St + y * Ct) / rows + 0.5;
	uint4 pixel = read_imageui(src, sampler, pos);
	write_imageui(dst, (int2)(ix, iy), pixel);
}


__kernel void convolution(sampler_t const sampler,
	__read_only image2d_t src, __write_only image2d_t dst, __constant float* filter,
	int const rows, int const cols, int const ksize)
{
	int const radius = ksize / 2;
	int2 sp, dp = {get_global_id(0), get_global_id(1)};
	float4 pixel = {0, 0, 0, 0};
	if (dp.x >= cols || dp.y >= rows)
		return;
	for (int y = 0; y < ksize; ++y)
	{
		sp.y = dp.y + y - radius;
		__constant float* fptr = filter + y * ksize;
		for (int x = 0; x < ksize; ++x)
		{
			sp.x = dp.x + x - radius;
			uint4 val = read_imageui(src, sampler, sp);
			pixel += convert_float4(val) * fptr[x];
		}
	}
	write_imageui(dst, dp, convert_uint4_rte(pixel));
}

