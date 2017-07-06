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
    class Swapchain;

    struct DepthStencil
    {
        Resource<VkImage> image;
        Resource<VkDeviceMemory> mem;
        Resource<VkImageView> view;
    };

    struct PhysicalDevice
    {
        VkPhysicalDevice device;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memoryProperties;
    };

    auto getPhysicalDevice(VkInstance instance) -> VkPhysicalDevice;
    auto createDevice(VkPhysicalDevice physicalDevice, uint32_t queueIndex) -> Resource<VkDevice>;
    auto getSurfaceFormats(VkPhysicalDevice device, VkSurfaceKHR surface) -> std::tuple<VkFormat, VkColorSpaceKHR>;
    auto getQueueIndex(VkPhysicalDevice device, VkSurfaceKHR surface) -> uint32_t;
    auto getDepthFormat(VkPhysicalDevice device) -> VkFormat;
    auto createCommandPool(VkDevice device, uint32_t queueIndex) -> Resource<VkCommandPool>;
    auto createDepthStencil(VkDevice device, const PhysicalDevice &physicalDevice,
        VkFormat depthFormat, uint32_t canvasWidth, uint32_t canvasHeight) -> DepthStencil;
    auto findMemoryType(const PhysicalDevice &physicalDevice, uint32_t typeBits, VkMemoryPropertyFlags properties) -> int32_t;
    auto createFrameBuffer(VkDevice device, VkImageView colorAttachment, VkImageView depthAttachment,
        VkRenderPass renderPass, uint32_t width, uint32_t height) -> Resource<VkFramebuffer>;
    auto createSemaphore(VkDevice device) -> Resource<VkSemaphore>;
    void createCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t count, VkCommandBuffer *result);
    auto createCommandBuffer(VkDevice device, VkCommandPool commandPool) -> VkCommandBuffer;
    auto createShader(VkDevice device, const void *data, uint32_t size) -> Resource<VkShaderModule>;
    auto createShaderStageInfo(bool vertex, VkShaderModule shader, const char *entryPoint) -> VkPipelineShaderStageCreateInfo;
    void queueSubmit(VkQueue queue, uint32_t waitSemaphoreCount, const VkSemaphore *waitSemaphores,
        uint32_t signalSemaphoreCount, const VkSemaphore *signalSemaphores,
        uint32_t commandBufferCount, const VkCommandBuffer *commandBuffers);
    void queuePresent(VkQueue queue, const vk::Swapchain &swapchain, uint32_t swapchainStep,
        uint32_t waitSemaphoreCount, const VkSemaphore *waitSemaphores);
    void setImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
        VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
    auto createDebugCallback(VkInstance instance, PFN_vkDebugReportCallbackEXT callbackFunc) -> VkDebugReportCallbackEXT;
    void beginCommandBuffer(VkCommandBuffer buffer, bool oneTime);
    auto createImage(VkDevice device, VkFormat format, uint32_t width, uint32_t height, uint32_t mipLevels,
        uint32_t arrayLayers, VkImageCreateFlags createFlags, VkImageUsageFlags usageFlags) -> vk::Resource<VkImage>;
    auto createImageView(VkDevice device, VkFormat format, VkImageViewType type, uint32_t mipLevels, uint32_t layers,
        VkImage image, VkImageAspectFlags aspectMask) -> vk::Resource<VkImageView>;
    auto allocateImageMemory(VkDevice device, VkImage image, vk::PhysicalDevice physicalDevice) -> vk::Resource<VkDeviceMemory>;
}
