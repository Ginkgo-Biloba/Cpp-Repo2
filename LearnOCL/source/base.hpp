#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <libjpeg/jpeglib.h>
#include <opencv2/core.hpp>
#include <CL/cl.h>
#undef NDEBUG
#include <cassert>
using cv::AutoBuffer;
using cv::Mat;
using std::string;
using std::vector;
typedef cv::Vec<size_t, 4> Vec4z;


char const* clErrorString(int err)
{
#define CL_ERR_STR(x) \
	case x: return #x

	switch (err)
	{
		/* Error Codes */
		CL_ERR_STR(CL_SUCCESS);
		CL_ERR_STR(CL_DEVICE_NOT_FOUND);
		CL_ERR_STR(CL_DEVICE_NOT_AVAILABLE);
		CL_ERR_STR(CL_COMPILER_NOT_AVAILABLE);
		CL_ERR_STR(CL_MEM_OBJECT_ALLOCATION_FAILURE);
		CL_ERR_STR(CL_OUT_OF_RESOURCES);
		CL_ERR_STR(CL_OUT_OF_HOST_MEMORY);
		CL_ERR_STR(CL_PROFILING_INFO_NOT_AVAILABLE);
		CL_ERR_STR(CL_MEM_COPY_OVERLAP);
		CL_ERR_STR(CL_IMAGE_FORMAT_MISMATCH);
		CL_ERR_STR(CL_IMAGE_FORMAT_NOT_SUPPORTED);
		CL_ERR_STR(CL_BUILD_PROGRAM_FAILURE);
		CL_ERR_STR(CL_MAP_FAILURE);
		CL_ERR_STR(CL_MISALIGNED_SUB_BUFFER_OFFSET);
		CL_ERR_STR(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
		CL_ERR_STR(CL_COMPILE_PROGRAM_FAILURE);
		CL_ERR_STR(CL_LINKER_NOT_AVAILABLE);
		CL_ERR_STR(CL_LINK_PROGRAM_FAILURE);
		CL_ERR_STR(CL_DEVICE_PARTITION_FAILED);
		CL_ERR_STR(CL_KERNEL_ARG_INFO_NOT_AVAILABLE);
		CL_ERR_STR(CL_INVALID_VALUE);
		CL_ERR_STR(CL_INVALID_DEVICE_TYPE);
		CL_ERR_STR(CL_INVALID_PLATFORM);
		CL_ERR_STR(CL_INVALID_DEVICE);
		CL_ERR_STR(CL_INVALID_CONTEXT);
		CL_ERR_STR(CL_INVALID_QUEUE_PROPERTIES);
		CL_ERR_STR(CL_INVALID_COMMAND_QUEUE);
		CL_ERR_STR(CL_INVALID_HOST_PTR);
		CL_ERR_STR(CL_INVALID_MEM_OBJECT);
		CL_ERR_STR(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
		CL_ERR_STR(CL_INVALID_IMAGE_SIZE);
		CL_ERR_STR(CL_INVALID_SAMPLER);
		CL_ERR_STR(CL_INVALID_BINARY);
		CL_ERR_STR(CL_INVALID_BUILD_OPTIONS);
		CL_ERR_STR(CL_INVALID_PROGRAM);
		CL_ERR_STR(CL_INVALID_PROGRAM_EXECUTABLE);
		CL_ERR_STR(CL_INVALID_KERNEL_NAME);
		CL_ERR_STR(CL_INVALID_KERNEL_DEFINITION);
		CL_ERR_STR(CL_INVALID_KERNEL);
		CL_ERR_STR(CL_INVALID_ARG_INDEX);
		CL_ERR_STR(CL_INVALID_ARG_VALUE);
		CL_ERR_STR(CL_INVALID_ARG_SIZE);
		CL_ERR_STR(CL_INVALID_KERNEL_ARGS);
		CL_ERR_STR(CL_INVALID_WORK_DIMENSION);
		CL_ERR_STR(CL_INVALID_WORK_GROUP_SIZE);
		CL_ERR_STR(CL_INVALID_WORK_ITEM_SIZE);
		CL_ERR_STR(CL_INVALID_GLOBAL_OFFSET);
		CL_ERR_STR(CL_INVALID_EVENT_WAIT_LIST);
		CL_ERR_STR(CL_INVALID_EVENT);
		CL_ERR_STR(CL_INVALID_OPERATION);
		CL_ERR_STR(CL_INVALID_GL_OBJECT);
		CL_ERR_STR(CL_INVALID_BUFFER_SIZE);
		CL_ERR_STR(CL_INVALID_MIP_LEVEL);
		CL_ERR_STR(CL_INVALID_GLOBAL_WORK_SIZE);
		CL_ERR_STR(CL_INVALID_PROPERTY);
		CL_ERR_STR(CL_INVALID_IMAGE_DESCRIPTOR);
		CL_ERR_STR(CL_INVALID_COMPILER_OPTIONS);
		CL_ERR_STR(CL_INVALID_LINKER_OPTIONS);
		CL_ERR_STR(CL_INVALID_DEVICE_PARTITION_COUNT);
	}
	return "Unknown Error";
#undef CL_ERR_STR
}


void checkCLError(int err, int line, char const* file, char const* info)
{
	if (!err)
		return;
	fprintf(stderr,
		"OpenCL Error %d (%s) at line %d in\n\tFile: %s\n\tExpr: %s\n",
		err, clErrorString(err), line, file, info);
	fflush(stdout), fflush(stderr);
	static_cast<int*>(NULL)[0] = 0x2b;
}

#define CheckCLError(expr) \
	expr;                    \
	checkCLError(err, __LINE__, __FILE__, #expr)


void getCLTime(cl_event E, char const* info)
{
	cl_ulong t, q;
	clGetEventProfilingInfo(E, CL_PROFILING_COMMAND_START, sizeof(t), &t, NULL);
	clGetEventProfilingInfo(E, CL_PROFILING_COMMAND_END, sizeof(q), &q, NULL);
	fprintf(stderr, "%s: %.2fms\n", info, (q - t) * 1e-6);
	fflush(stderr);
}


string loadCLFile(char const* file)
{
	string K;
	FILE* f = fopen(file, "rb");
	fseek(f, 0, SEEK_END);
	int flen = ftell(f);
	K.resize(flen + 2, 0);
	fseek(f, 0, SEEK_SET);
	fread(&K[0], 1, flen, f);
	fclose(f);
	return K;
}


void makeDiv(Vec4z& total, Vec4z& local)
{
	for (int i = 0; i < Vec4z::channels; ++i)
		if (local[i])
			total[i] = (total[i] + local[i] - 1) / local[i] * local[i];
}


bool jpgRead(char const* name, Mat& src)
{
	FILE* fid = fopen(name, "rb");
	if (!fid)
	{
		fprintf(stderr, "can't open %s\n", name);
		return false;
	}

	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, fid);
	jpeg_read_header(&cinfo, TRUE);

	int cn = cinfo.num_components;
	if (cn == 1)
	{
		cinfo.out_color_components = 1;
		cinfo.out_color_space = JCS_GRAYSCALE;
	}
	else if (cn == 3)
	{
		cinfo.out_color_components = 3;
		cinfo.out_color_space = JCS_RGB;
	}
	else
		CV_Assert(cn == 1 || cn == 3);

	jpeg_start_decompress(&cinfo);
	src.create(cinfo.output_height, cinfo.output_width, CV_MAKETYPE(CV_8U, cn));
	while (cinfo.output_scanline < cinfo.output_height)
	{
		uchar* ptr = src.ptr(cinfo.output_scanline);
		jpeg_read_scanlines(&cinfo, &ptr, 1);
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(fid);
	return true;
}


bool jpgWrite(char const* name, Mat const& src)
{
	int cn = src.channels();
	CV_Assert(cn == 1 || cn == 3);
	CV_Assert(src.depth() == CV_8U);

	FILE* fid = fopen(name, "wb");
	if (!fid)
	{
		fprintf(stderr, "can't open %s\n", name);
		return false;
	}

	jpeg_compress_struct cinfo;
	jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	cinfo.image_height = src.rows;
	cinfo.image_width = src.cols;
	cinfo.input_components = cn;
	cinfo.in_color_space = (cn == 1 ? JCS_GRAYSCALE : JCS_RGB);
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 85, TRUE);
	for (int i = 0; i < 3; ++i)
		cinfo.comp_info[i].h_samp_factor = cinfo.comp_info[i].v_samp_factor = 1;

	jpeg_stdio_dest(&cinfo, fid);
	jpeg_start_compress(&cinfo, TRUE);
	while (cinfo.next_scanline < cinfo.image_height)
	{
		uchar* ptr = const_cast<uchar*>(src.ptr(cinfo.next_scanline));
		jpeg_write_scanlines(&cinfo, &ptr, 1);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	fclose(fid);
	return true;
}
