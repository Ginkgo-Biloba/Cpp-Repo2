// Enable the WSI extensions
#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
using std::vector;
using std::string;


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerCreateInfoEXT* crtinfo,
	VkAllocationCallbacks* allocator,
	VkDebugUtilsMessengerEXT* message)
{
	auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
	if (func)
		return func(instance, crtinfo, allocator, message);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}


void DestroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT message,
	VkAllocationCallbacks* allocator)
{
	auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>
		(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (func)
		func(instance, message, allocator);
}


VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	VkDebugUtilsMessengerCallbackDataEXT const* data,
	void* user)
{
	_CRT_UNUSED(severity);
	_CRT_UNUSED(type);
	_CRT_UNUSED(user);
	printf("%s : %s\n", data->pMessageIdName, data->pMessage);
	return VK_FALSE;
}


class HelloVulkan
{
	// 假设仅适用于支持 geometry shaders 的专用显卡
	bool is_device_suitable(VkPhysicalDevice dev);
	bool check_validation_layer_support();
	unsigned findQueueFamily(VkPhysicalDevice dev);

	vector<VkPhysicalDevice> alldevice;
	vector<string> extneed;
	vector<string> validlayer;
	vector<VkLayerProperties> alllayer;
	vector<VkQueueFamilyProperties> allfamily;
	VkBool32 enable_validation_layer;

public:
	GLFWwindow* window;
	VkInstance instance;
	VkAllocationCallbacks* allocator;
	VkDebugUtilsMessengerEXT debugmessage;
	VkPhysicalDevice physical;
	VkDevice device;
	VkQueue graphyqueue;
	vector<VkExtensionProperties> extension;
	unsigned idxfamily;

	HelloVulkan();
	~HelloVulkan();

	VkResult create();
	VkResult release();

	void run();
};


bool HelloVulkan::is_device_suitable(VkPhysicalDevice dev)
{
	/// inquire physical device's informations
	VkPhysicalDeviceProperties prop;
	VkPhysicalDeviceFeatures feat;
	vkGetPhysicalDeviceProperties(dev, &prop);
	vkGetPhysicalDeviceFeatures(dev, &feat);
	if (feat.geometryShader
		&& (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			|| prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU))
	{
		printf("Select GPU %s (vendor id %u)\n",
			prop.deviceName, prop.vendorID);
	}
	else
		return false;

	unsigned quenum = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &quenum, NULL);
	allfamily.resize(quenum);
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &quenum, allfamily.data());

	idxfamily = UINT_MAX;
	for (unsigned i = 0; i < quenum; ++i)
		if (allfamily[i].queueCount > 0
			&& (allfamily[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			idxfamily = i;
			break;
		}

	return idxfamily < 16;
}


bool HelloVulkan::check_validation_layer_support()
{
	unsigned nlayer = 0;
	vkEnumerateInstanceLayerProperties(&nlayer, NULL);
	alllayer.resize(nlayer);
	vkEnumerateInstanceLayerProperties(&nlayer, alllayer.data());

	for (string& s : validlayer)
	{
		bool found = false;
		for (auto& layer : alllayer)
			if (s == layer.layerName)
			{
				found = true;
				break;
			}
		if (!found)
			return false;
	}
	return true;
}


HelloVulkan::HelloVulkan()
{
	enable_validation_layer = VK_TRUE;
	// validlayer.emplace_back("VK_LAYER_KHRONOS_validation");
	validlayer.emplace_back("VK_LAYER_LUNARG_core_validation");
	validlayer.emplace_back("VK_LAYER_LUNARG_object_tracker");
	validlayer.emplace_back("VK_LAYER_LUNARG_parameter_validation");
	validlayer.emplace_back("VK_LAYER_GOOGLE_threading");
	validlayer.emplace_back("VK_LAYER_GOOGLE_unique_objects");

	window = NULL;
	physical = VK_NULL_HANDLE;
	device = VK_NULL_HANDLE;
	instance = VK_NULL_HANDLE;
	allocator = NULL;
	idxfamily = INT_MAX;
}


HelloVulkan::~HelloVulkan()
{
	release();
}


VkResult HelloVulkan::create()
{
	/// glfw
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);

	/// extensions glfw needs
	unsigned number = 0;
	char const** glfwext = glfwGetRequiredInstanceExtensions(&number);
	for (unsigned i = 0; i < number; ++i)
		extneed.emplace_back(glfwext[i]);

	/// validation layer
	VkResult vkret = VK_SUCCESS;
	if (enable_validation_layer)
		extneed.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	bool layersupported = check_validation_layer_support();
	if (enable_validation_layer && !layersupported)
	{
		puts("validation layers requested, but not available!");
		return VK_ERROR_LAYER_NOT_PRESENT;
	}

	/// supported extensions
	number = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &number, nullptr);
	extension.resize(number);
	vkEnumerateInstanceExtensionProperties(nullptr, &number, extension.data());

	/// create instance info
	VkApplicationInfo appinfo;
	memset(&appinfo, 0, sizeof(appinfo));
	appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appinfo.pApplicationName = "AppInfo";
	appinfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appinfo.pEngineName = "NoEngine";
	appinfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appinfo.apiVersion = VK_API_VERSION_1_0;

	vector<char const*> extneedptr;
	for (string& s : extneed)
		extneedptr.push_back(s.c_str());

	VkInstanceCreateInfo crtinfo;
	memset(&crtinfo, 0, sizeof(crtinfo));
	crtinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	crtinfo.pApplicationInfo = &appinfo;
	crtinfo.enabledExtensionCount = static_cast<unsigned>(extneedptr.size());
	crtinfo.ppEnabledExtensionNames = extneedptr.data();

	/// debug info
	VkDebugUtilsMessengerCreateInfoEXT debuginfo;
	vector<char const*> validptr;
	if (enable_validation_layer)
	{
		for (string& s : validlayer)
			validptr.push_back(s.c_str());
		memset(&debuginfo, 0, sizeof(debuginfo));
		debuginfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debuginfo.messageSeverity = 0
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debuginfo.messageType = 0
			| VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debuginfo.pfnUserCallback = DebugCallback;
		crtinfo.pNext = &debuginfo;
	}
	crtinfo.enabledLayerCount = static_cast<unsigned>(validptr.size());
	crtinfo.ppEnabledLayerNames = validptr.data();

	/// create instance
	vkret = vkCreateInstance(&crtinfo, nullptr, &instance);
	if (vkret != VK_SUCCESS)
	{
		printf("vkCreateInstance failed with %d\n", vkret);
		return vkret;
	}

	/// debug callback
	if (enable_validation_layer)
	{
		vkret = CreateDebugUtilsMessengerEXT(instance, &debuginfo, allocator, &debugmessage);
		if (vkret != VK_SUCCESS)
		{
			printf("CreateDebugUtilsMessengerEXT failed with %d\n", vkret);
			return VK_ERROR_INVALID_EXTERNAL_HANDLE;
		}
	}

	/// pick up first physical device
	unsigned devnum = 1;
	vkEnumeratePhysicalDevices(instance, &devnum, nullptr);
	if (!devnum)
	{
		puts("Can not file any GPU");
		return VK_ERROR_DEVICE_LOST;
	}
	alldevice.resize(devnum);
	vkEnumeratePhysicalDevices(instance, &devnum, alldevice.data());
	for (VkPhysicalDevice d : alldevice)
		if (is_device_suitable(d))
		{
			physical = d;
			break;
		}
	if (!physical)
	{
		puts("Can not file any suitable GPU");
		return VK_ERROR_DEVICE_LOST;
	}

	/// createLogicalDevice
	VkDeviceQueueCreateInfo queinfo;
	memset(&queinfo, 0, sizeof(queinfo));
	queinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queinfo.queueFamilyIndex = idxfamily;
	queinfo.queueCount = 1;

	float queuepriority = 1.F;
	queinfo.pQueuePriorities = &queuepriority;

	VkPhysicalDeviceFeatures devfeat;
	memset(&devfeat, 0, sizeof(devfeat));
	VkDeviceCreateInfo devinfo;
	memset(&devinfo, 0, sizeof(devinfo));
	devinfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	devinfo.pQueueCreateInfos = &queinfo;
	devinfo.queueCreateInfoCount = 1;
	devinfo.pEnabledFeatures = &devfeat;
	devinfo.enabledExtensionCount = 0;
	devinfo.enabledLayerCount = static_cast<unsigned>(validptr.size());
	devinfo.ppEnabledLayerNames = validptr.data();

	vkret = vkCreateDevice(physical, &devinfo, allocator, &device);
	if (vkret != VK_SUCCESS)
	{
		puts("Failed to create logical device!");
		return VK_ERROR_DEVICE_LOST;
	}
	vkGetDeviceQueue(device, idxfamily, 0, &graphyqueue);

	return VK_SUCCESS;
}


VkResult HelloVulkan::release()
{
	if (enable_validation_layer)
		DestroyDebugUtilsMessengerEXT(instance, debugmessage, allocator);
	if (instance)
		vkDestroyInstance(instance, nullptr);
	if (window)
		glfwDestroyWindow(window);
	glfwTerminate();
	window = NULL;
	physical = VK_NULL_HANDLE;
	device = VK_NULL_HANDLE;
	instance = VK_NULL_HANDLE;
	allocator = NULL;
	idxfamily = INT_MAX;
	return VK_SUCCESS;
}


void HelloVulkan::run()
{
	while (!glfwWindowShouldClose(window))
	{
		Sleep(100);
		glfwPollEvents();
	}
}


int main()
{
	HelloVulkan app;
	VkResult ret = app.create();
	if (ret != VK_SUCCESS)
	{
		printf("create app failed with %d\n", ret);
		return ret;
	}

	app.run();

	return 0;
}

