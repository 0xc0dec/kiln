/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vulkan.h"
#include "../Common.h"
#include <vector>

namespace vk
{
    class Device
    {
    public:
#ifdef KL_WINDOWS
        static auto create(const std::vector<uint8_t> &platformHandle) -> Device;
#endif

        auto getInstance() const -> VkInstance { return instance; }
        auto getSurface() const -> VkSurfaceKHR { return surface; }
        auto getPhysicalDevice() const -> VkPhysicalDevice { return physicalDevice; }
        auto getPhysicalFeatures() const -> VkPhysicalDeviceFeatures { return physicalFeatures; }
        auto getPhysicalProperties() const -> VkPhysicalDeviceProperties { return physicalProperties; }
        auto getPhysicalMemoryFeatures() const -> VkPhysicalDeviceMemoryProperties { return physicalMemoryFeatures; }
        auto getColorFormat() const -> VkFormat { return colorFormat; }
        auto getDepthFormat() const -> VkFormat { return depthFormat; }
        auto getColorSpace() const -> VkColorSpaceKHR { return colorSpace; }
        auto getCommandPool() const -> VkCommandPool { return commandPool; }
        auto getQueue() const -> VkQueue { return queue; }

        operator VkDevice() { return device; }
        operator VkDevice() const { return device; }

    private:
        Resource<VkInstance> instance;
        Resource<VkSurfaceKHR> surface;
        Resource<VkDebugReportCallbackEXT> debugCallback;
        Resource<VkDevice> device;
        Resource<VkCommandPool> commandPool;
        VkPhysicalDevice physicalDevice = nullptr;
        VkPhysicalDeviceFeatures physicalFeatures{};
        VkPhysicalDeviceProperties physicalProperties{};
        VkPhysicalDeviceMemoryProperties physicalMemoryFeatures{};
        VkFormat colorFormat = VK_FORMAT_UNDEFINED;
        VkFormat depthFormat = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR;
        VkQueue queue = nullptr;

        Device() {}
    };
}