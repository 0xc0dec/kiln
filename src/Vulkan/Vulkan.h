/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "../Common.h"
#include "VulkanResource.h"

#ifdef KL_WINDOWS
#   define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan.h>
#include <functional>

#ifdef KL_DEBUG
#   define KL_VK_CHECK_RESULT(vkCall, ...) KL_PANIC_IF(vkCall != VK_SUCCESS, __VA_ARGS__)
#else
#   define KL_VK_CHECK_RESULT(vkCall) vkCall
#endif

namespace vk
{
    struct DepthStencil
    {
        Resource<VkImage> image;
        Resource<VkDeviceMemory> mem;
        Resource<VkImageView> view;
    };

    auto getPhysicalDevice(VkInstance instance) -> VkPhysicalDevice;
    auto createDevice(VkPhysicalDevice physicalDevice, uint32_t queueIndex) -> Resource<VkDevice>;
    auto getSurfaceFormats(VkPhysicalDevice device, VkSurfaceKHR surface) -> std::tuple<VkFormat, VkColorSpaceKHR>;
    auto getQueueIndex(VkPhysicalDevice device, VkSurfaceKHR surface) -> uint32_t;
    auto getDepthFormat(VkPhysicalDevice device) -> VkFormat;
    auto createCommandPool(VkDevice device, uint32_t queueIndex) -> Resource<VkCommandPool>;
    auto createDepthStencil(VkDevice device, VkPhysicalDeviceMemoryProperties physicalDeviceMemProps,
        VkFormat depthFormat, uint32_t canvasWidth, uint32_t canvasHeight) -> DepthStencil;
    auto findMemoryType(VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties, uint32_t typeBits,
        VkMemoryPropertyFlags properties) -> int32_t;
    auto createFrameBuffer(VkDevice device, VkImageView colorAttachment, VkImageView depthAttachment,
        VkRenderPass renderPass, uint32_t width, uint32_t height) -> Resource<VkFramebuffer>;
    auto createSemaphore(VkDevice device) -> Resource<VkSemaphore>;
    void createCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t count, VkCommandBuffer *result);
    auto createShader(VkDevice device, const void *data, uint32_t size) -> Resource<VkShaderModule>;
    auto createShaderStageInfo(bool vertex, VkShaderModule shader, const char *entryPoint) -> VkPipelineShaderStageCreateInfo;
}
