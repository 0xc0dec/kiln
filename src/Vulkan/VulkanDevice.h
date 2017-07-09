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

        auto getInstance() const -> VkInstance;
        auto getSurface() const -> VkSurfaceKHR;
        auto getPhysicalDevice() const -> VkPhysicalDevice;
        auto getPhysicalFeatures() const -> VkPhysicalDeviceFeatures;
        auto getPhysicalProperties() const -> VkPhysicalDeviceProperties;
        auto getPhysicalMemoryFeatures() const -> VkPhysicalDeviceMemoryProperties;
        auto getColorFormat() const -> VkFormat;
        auto getDepthFormat() const -> VkFormat;
        auto getColorSpace() const -> VkColorSpaceKHR;
        auto getCommandPool() const -> VkCommandPool;
        auto getQueue() const -> VkQueue;

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

    inline auto Device::getInstance() const -> VkInstance
    {
        return instance;
    }

    inline auto Device::getSurface() const -> VkSurfaceKHR
    {
        return surface;
    }

    inline auto Device::getPhysicalDevice() const -> VkPhysicalDevice
    {
        return physicalDevice;
    }

    inline auto Device::getColorFormat() const -> VkFormat
    {
        return colorFormat;
    }

    inline auto Device::getDepthFormat() const -> VkFormat
    {
        return depthFormat;
    }

    inline auto Device::getColorSpace() const -> VkColorSpaceKHR
    {
        return colorSpace;
    }

    inline auto Device::getCommandPool() const -> VkCommandPool
    {
        return commandPool;
    }

    inline auto Device::getQueue() const -> VkQueue
    {
        return queue;
    }

    inline auto Device::getPhysicalFeatures() const -> VkPhysicalDeviceFeatures
    {
        return physicalFeatures;
    }

    inline auto Device::getPhysicalProperties() const -> VkPhysicalDeviceProperties
    {
        return physicalProperties;
    }

    inline auto Device::getPhysicalMemoryFeatures() const -> VkPhysicalDeviceMemoryProperties
    {
        return physicalMemoryFeatures;
    }
}