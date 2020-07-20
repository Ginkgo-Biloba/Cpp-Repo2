#pragma once

#include <algorithm>
#include <vector>
#include <string>
using std::string;
using std::vector;

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>


struct Platform
{
	cl_platform_id handle;
	string vendor;

	Platform(cl_platform_id d)
	{
		handle = d;
		char buf[128];
		size_t sz = 0;
		clGetPlatformInfo(handle, CL_PLATFORM_VENDOR, sizeof(buf), buf, &sz);
		if (sz < sizeof(buf))
		{
			vendor.append(buf);
			vendor.append(" | ");
		}
		clGetPlatformInfo(handle, CL_PLATFORM_NAME, sizeof(buf), buf, &sz);
		if (sz < sizeof(buf))
			vendor.append(buf);
	}


	static void get(vector<Platform>& plat)
	{
		cl_platform_id id[16];
		cl_uint nid = 0;
		if (clGetPlatformIDs(16, id, &nid) != CL_SUCCESS)
			return;

		for (cl_uint i = 0; i < nid; ++i)
			plat.push_back(Platform(id[i]));
	}
};



struct Device
{
	cl_device_id handle;
	string name;
	string version;
	string extensions;
	vector<string> extensionSet;
	int doubleFPConfig;
	bool hostUnifiedMemory;
	int maxComputeUnits;
	size_t maxWorkGroupSize;
	int type;
	int addressBits;
	int deviceVersionMajor;
	int deviceVersionMinor;
	string driverVersion;
	string vendorName;
	int vendorID;
	bool intelSubgroupsSupport;

	// deviceVersion has format
	// OpenCL<space><major_version.minor_version><space><vendor-specific information>
	// by specification
	// http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clGetDeviceInfo.html
	// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetDeviceInfo.html
	static void parseDeviceVersion(string const& ver, int& major, int& minor)
	{
		major = minor = 0;
		if (10 >= ver.length())
			return;
		const char* pstr = ver.c_str();
		if (0 != strncmp(pstr, "OpenCL ", 7))
			return;
		size_t ppos = ver.find('.', 7);
		if (string::npos == ppos)
			return;
		string temp = ver.substr(7, ppos - 7);
		major = atoi(temp.c_str());
		temp = ver.substr(ppos + 1);
		minor = atoi(temp.c_str());
	}

	enum
	{
		TYPE_DEFAULT = (1 << 0),
		TYPE_CPU = (1 << 1),
		TYPE_GPU = (1 << 2),
		TYPE_ACCELERATOR = (1 << 3),
		TYPE_DGPU = TYPE_GPU + (1 << 16),
		TYPE_IGPU = TYPE_GPU + (1 << 17),
		TYPE_ALL = 0xFFFFFFFF,

		FP_DENORM = (1 << 0),
		FP_INF_NAN = (1 << 1),
		FP_ROUND_TO_NEAREST = (1 << 2),
		FP_ROUND_TO_ZERO = (1 << 3),
		FP_ROUND_TO_INF = (1 << 4),
		FP_FMA = (1 << 5),
		FP_SOFT_FLOAT = (1 << 6),
		FP_CORRECTLY_ROUNDED_DIVIDE_SQRT = (1 << 7),

		EXEC_KERNEL = (1 << 0),
		EXEC_NATIVE_KERNEL = (1 << 1),

		NO_CACHE = 0,
		READ_ONLY_CACHE = 1,
		READ_WRITE_CACHE = 2,

		NO_LOCAL_MEM = 0,
		LOCAL_IS_LOCAL = 1,
		LOCAL_IS_GLOBAL = 2,

		UNKNOWN_VENDOR = 0,
		VENDOR_AMD = 1,
		VENDOR_INTEL = 2,
		VENDOR_NVIDIA = 3
	};

	template<typename _TpCL, typename _TpOut>
	_TpOut getProp(cl_device_info prop) const
	{
		_TpCL temp = _TpCL();
		size_t sz = 0;

		return clGetDeviceInfo(handle, prop, sizeof(temp), &temp, &sz) == CL_SUCCESS &&
			sz == sizeof(temp) ? _TpOut(temp) : _TpOut();
	}

	bool getBoolProp(cl_device_info prop) const
	{
		cl_bool temp = CL_FALSE;
		size_t sz = 0;

		return clGetDeviceInfo(handle, prop, sizeof(temp), &temp, &sz) == CL_SUCCESS &&
			sz == sizeof(temp) ? temp != 0 : false;
	}

	string getStrProp(cl_device_info prop) const
	{
		string s;
		char buf[4096];
		size_t sz = 0;
		if (sz < sizeof(buf)
			&& clGetDeviceInfo(handle, prop, sizeof(buf) - 16, buf, &sz) == CL_SUCCESS)
			s.append(buf);
		return s;
	}

	bool hasExtension(string const& ext) const
	{
		return
			binary_search(extensionSet.begin(), extensionSet.end(), ext);
	}

	Device(cl_device_id d)
	{
		handle = d;

		name = getStrProp(CL_DEVICE_NAME);
		version = getStrProp(CL_DEVICE_VERSION);
		extensions = getStrProp(CL_DEVICE_EXTENSIONS);
		doubleFPConfig = getProp<cl_device_fp_config, int>(CL_DEVICE_DOUBLE_FP_CONFIG);
		hostUnifiedMemory = getBoolProp(CL_DEVICE_HOST_UNIFIED_MEMORY);
		maxComputeUnits = getProp<cl_uint, int>(CL_DEVICE_MAX_COMPUTE_UNITS);
		maxWorkGroupSize = getProp<size_t, size_t>(CL_DEVICE_MAX_WORK_GROUP_SIZE);
		type = getProp<cl_device_type, int>(CL_DEVICE_TYPE);
		driverVersion = getStrProp(CL_DRIVER_VERSION);
		addressBits = getProp<cl_uint, int>(CL_DEVICE_ADDRESS_BITS);

		string deviceVersion_ = getStrProp(CL_DEVICE_VERSION);
		parseDeviceVersion(deviceVersion_, deviceVersionMajor, deviceVersionMinor);

		size_t pos = 0;
		while (pos < extensions.size())
		{
			size_t pos2 = extensions.find(' ', pos);
			if (pos2 == string::npos)
				pos2 = extensions.size();
			if (pos2 > pos)
			{
				string extensionName = extensions.substr(pos, pos2 - pos);
				extensionSet.push_back(extensionName);
			}
			pos = pos2 + 1;
		}
		sort(extensionSet.begin(), extensionSet.end());

		intelSubgroupsSupport = hasExtension("cl_intel_subgroups");

		vendorName = getStrProp(CL_DEVICE_VENDOR);
		if (vendorName == "Advanced Micro Devices, Inc." ||
			vendorName == "AMD")
			vendorID = VENDOR_AMD;
		else if (vendorName == "Intel(R) Corporation"
			|| vendorName == "Intel"
			|| strstr(name.c_str(), "Iris") != 0)
			vendorID = VENDOR_INTEL;
		else if (vendorName == "NVIDIA Corporation")
			vendorID = VENDOR_NVIDIA;
		else
			vendorID = UNKNOWN_VENDOR;
	}


	static void get(Platform& plat, vector<Device>& dev)
	{
		cl_device_id id[16];
		cl_uint nd = 0;

		if (clGetDeviceIDs(plat.handle, CL_DEVICE_TYPE_ALL, 16, id, &nd)
			!= CL_SUCCESS)
			return;

		for (cl_uint i = 0; i < nd; ++i)
			dev.push_back(Device(id[i]));
	}
};


const char* getOpenCLErrorString(int errorCode)
{
#define CV_OCL_CODE(id) case id: return #id
#define CV_OCL_CODE_(id, name) case id: return #name
	switch (errorCode)
	{
		CV_OCL_CODE(CL_SUCCESS);
		CV_OCL_CODE(CL_DEVICE_NOT_FOUND);
		CV_OCL_CODE(CL_DEVICE_NOT_AVAILABLE);
		CV_OCL_CODE(CL_COMPILER_NOT_AVAILABLE);
		CV_OCL_CODE(CL_MEM_OBJECT_ALLOCATION_FAILURE);
		CV_OCL_CODE(CL_OUT_OF_RESOURCES);
		CV_OCL_CODE(CL_OUT_OF_HOST_MEMORY);
		CV_OCL_CODE(CL_PROFILING_INFO_NOT_AVAILABLE);
		CV_OCL_CODE(CL_MEM_COPY_OVERLAP);
		CV_OCL_CODE(CL_IMAGE_FORMAT_MISMATCH);
		CV_OCL_CODE(CL_IMAGE_FORMAT_NOT_SUPPORTED);
		CV_OCL_CODE(CL_BUILD_PROGRAM_FAILURE);
		CV_OCL_CODE(CL_MAP_FAILURE);
		CV_OCL_CODE(CL_MISALIGNED_SUB_BUFFER_OFFSET);
		CV_OCL_CODE(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
		CV_OCL_CODE(CL_COMPILE_PROGRAM_FAILURE);
		CV_OCL_CODE(CL_LINKER_NOT_AVAILABLE);
		CV_OCL_CODE(CL_LINK_PROGRAM_FAILURE);
		CV_OCL_CODE(CL_DEVICE_PARTITION_FAILED);
		CV_OCL_CODE(CL_KERNEL_ARG_INFO_NOT_AVAILABLE);
		CV_OCL_CODE(CL_INVALID_VALUE);
		CV_OCL_CODE(CL_INVALID_DEVICE_TYPE);
		CV_OCL_CODE(CL_INVALID_PLATFORM);
		CV_OCL_CODE(CL_INVALID_DEVICE);
		CV_OCL_CODE(CL_INVALID_CONTEXT);
		CV_OCL_CODE(CL_INVALID_QUEUE_PROPERTIES);
		CV_OCL_CODE(CL_INVALID_COMMAND_QUEUE);
		CV_OCL_CODE(CL_INVALID_HOST_PTR);
		CV_OCL_CODE(CL_INVALID_MEM_OBJECT);
		CV_OCL_CODE(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
		CV_OCL_CODE(CL_INVALID_IMAGE_SIZE);
		CV_OCL_CODE(CL_INVALID_SAMPLER);
		CV_OCL_CODE(CL_INVALID_BINARY);
		CV_OCL_CODE(CL_INVALID_BUILD_OPTIONS);
		CV_OCL_CODE(CL_INVALID_PROGRAM);
		CV_OCL_CODE(CL_INVALID_PROGRAM_EXECUTABLE);
		CV_OCL_CODE(CL_INVALID_KERNEL_NAME);
		CV_OCL_CODE(CL_INVALID_KERNEL_DEFINITION);
		CV_OCL_CODE(CL_INVALID_KERNEL);
		CV_OCL_CODE(CL_INVALID_ARG_INDEX);
		CV_OCL_CODE(CL_INVALID_ARG_VALUE);
		CV_OCL_CODE(CL_INVALID_ARG_SIZE);
		CV_OCL_CODE(CL_INVALID_KERNEL_ARGS);
		CV_OCL_CODE(CL_INVALID_WORK_DIMENSION);
		CV_OCL_CODE(CL_INVALID_WORK_GROUP_SIZE);
		CV_OCL_CODE(CL_INVALID_WORK_ITEM_SIZE);
		CV_OCL_CODE(CL_INVALID_GLOBAL_OFFSET);
		CV_OCL_CODE(CL_INVALID_EVENT_WAIT_LIST);
		CV_OCL_CODE(CL_INVALID_EVENT);
		CV_OCL_CODE(CL_INVALID_OPERATION);
		CV_OCL_CODE(CL_INVALID_GL_OBJECT);
		CV_OCL_CODE(CL_INVALID_BUFFER_SIZE);
		CV_OCL_CODE(CL_INVALID_MIP_LEVEL);
		CV_OCL_CODE(CL_INVALID_GLOBAL_WORK_SIZE);
		// OpenCL 1.1
		CV_OCL_CODE(CL_INVALID_PROPERTY);
		// OpenCL 1.2
		CV_OCL_CODE(CL_INVALID_IMAGE_DESCRIPTOR);
		CV_OCL_CODE(CL_INVALID_COMPILER_OPTIONS);
		CV_OCL_CODE(CL_INVALID_LINKER_OPTIONS);
		CV_OCL_CODE(CL_INVALID_DEVICE_PARTITION_COUNT);
		// OpenCL 2.0
		CV_OCL_CODE_(-69, CL_INVALID_PIPE_SIZE);
		CV_OCL_CODE_(-70, CL_INVALID_DEVICE_QUEUE);
		// Extensions
		CV_OCL_CODE_(-1000, CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR);
		CV_OCL_CODE_(-1001, CL_PLATFORM_NOT_FOUND_KHR);
		CV_OCL_CODE_(-1002, CL_INVALID_D3D10_DEVICE_KHR);
		CV_OCL_CODE_(-1003, CL_INVALID_D3D10_RESOURCE_KHR);
		CV_OCL_CODE_(-1004, CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR);
		CV_OCL_CODE_(-1005, CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR);
	default: return "Unknown OpenCL error";
	}
#undef CV_OCL_CODE
#undef CV_OCL_CODE_
}










