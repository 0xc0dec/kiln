/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "VulkanDevice.h"
#include <vector>
#ifdef KL_WINDOWS
#   include <windows.h>
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackFunc(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
    uint64_t obj, size_t location, int32_t code, const char *layerPrefix, const char *msg, void *userData)
{
    return VK_FALSE;
}

#ifdef KL_WINDOWS

auto vk::Device::create(const std::vector<uint8_t> &platformHandle) -> Device
{
    VkApplicationInfo appInfo {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "";
    appInfo.pEngineName = "";
    appInfo.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char*> enabledExtensions {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef KL_WINDOWS
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#ifdef KL_DEBUG
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
    };

    std::vector<const char *> enabledLayers {
#ifdef KL_DEBUG
        "VK_LAYER_LUNARG_standard_validation",
#endif
    };

    VkInstanceCreateInfo instanceInfo {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext = nullptr;
    instanceInfo.pApplicationInfo = &appInfo;

    if (!enabledLayers.empty())
    {
        instanceInfo.enabledLayerCount = enabledLayers.size();
        instanceInfo.ppEnabledLayerNames = enabledLayers.data();
    }

    if (!enabledExtensions.empty())
    {
        instanceInfo.enabledExtensionCount = enabledExtensions.size();
        instanceInfo.ppEnabledExtensionNames = enabledExtensions.data();
    }

    Resource<VkInstance> instance{vkDestroyInstance};
    KL_VK_CHECK_RESULT(vkCreateInstance(&instanceInfo, nullptr, instance.cleanRef()));

#ifdef KL_WINDOWS
    struct
    {
        HWND hWnd;
        HINSTANCE hInst;
    } handle;

    memcpy(&handle, platformHandle.data(), sizeof(handle));

    VkWin32SurfaceCreateInfoKHR surfaceInfo;
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.flags = 0;
    surfaceInfo.pNext = nullptr;
    surfaceInfo.hinstance = handle.hInst;
    surfaceInfo.hwnd = handle.hWnd;

    Resource<VkSurfaceKHR> surface{instance, vkDestroySurfaceKHR};
    KL_VK_CHECK_RESULT(vkCreateWin32SurfaceKHR(instance, &surfaceInfo, nullptr, surface.cleanRef()));
#endif

    Device device{};
    device.instance = std::move(instance);
    device.surface = std::move(surface);
    device.debugCallback = createDebugCallback(device.instance, debugCallbackFunc);
    
    device.physicalDevice.device = vk::getPhysicalDevice(device.instance);
    vkGetPhysicalDeviceProperties(device.physicalDevice.device, &device.physicalDevice.properties);
    vkGetPhysicalDeviceFeatures(device.physicalDevice.device, &device.physicalDevice.features);
    vkGetPhysicalDeviceMemoryProperties(device.physicalDevice.device, &device.physicalDevice.memoryProperties);

    auto surfaceFormats = getSurfaceFormats(device.physicalDevice.device, device.surface);
    device.colorFormat = std::get<0>(surfaceFormats);
    device.depthFormat = vk::getDepthFormat(device.physicalDevice.device);
    device.colorSpace = std::get<1>(surfaceFormats);

    auto queueIndex = getQueueIndex(device.physicalDevice.device, device.surface);
    device.device = createDevice(device.physicalDevice.device, queueIndex);
    vkGetDeviceQueue(device, queueIndex, 0, &device.queue);

    device.commandPool = createCommandPool(device, queueIndex);

    return device;
}

#endif
