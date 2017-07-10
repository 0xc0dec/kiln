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

#ifdef KL_DEBUG
#   define KL_VK_CHECK_RESULT(vkCall, ...) KL_PANIC_IF(vkCall != VK_SUCCESS, __VA_ARGS__)
#else
#   define KL_VK_CHECK_RESULT(vkCall) vkCall
#endif

namespace vk
{
    class Swapchain;

    auto findMemoryType(VkPhysicalDeviceMemoryProperties memProps, uint32_t typeBits, VkMemoryPropertyFlags properties) -> int32_t;
    auto createFrameBuffer(VkDevice device, VkImageView colorAttachment, VkImageView depthAttachment,
        VkRenderPass renderPass, uint32_t width, uint32_t height) -> Resource<VkFramebuffer>;
    auto createSemaphore(VkDevice device) -> Resource<VkSemaphore>;
    auto createCommandBuffer(VkDevice device, VkCommandPool commandPool) -> Resource<VkCommandBuffer>;
    auto createShader(VkDevice device, const void *data, size_t size) -> Resource<VkShaderModule>;
    auto createShaderStageInfo(bool vertex, VkShaderModule shader, const char *entryPoint) -> VkPipelineShaderStageCreateInfo;
    void queueSubmit(VkQueue queue, uint32_t waitSemaphoreCount, const VkSemaphore *waitSemaphores,
        uint32_t signalSemaphoreCount, const VkSemaphore *signalSemaphores,
        uint32_t commandBufferCount, const VkCommandBuffer *commandBuffers);
    void queuePresent(VkQueue queue, const Swapchain &swapchain, uint32_t swapchainStep,
        uint32_t waitSemaphoreCount, const VkSemaphore *waitSemaphores);
    void setImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
        VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
    auto createDebugCallback(VkInstance instance, PFN_vkDebugReportCallbackEXT callbackFunc) -> Resource<VkDebugReportCallbackEXT>;
    void beginCommandBuffer(VkCommandBuffer buffer, bool oneTime);
    auto createImage(VkDevice device, VkFormat format, uint32_t width, uint32_t height, uint32_t mipLevels,
        uint32_t arrayLayers, VkImageCreateFlags createFlags, VkImageUsageFlags usageFlags) -> Resource<VkImage>;
    auto createImageView(VkDevice device, VkFormat format, VkImageViewType type, uint32_t mipLevels, uint32_t layers,
        VkImage image, VkImageAspectFlags aspectMask) -> Resource<VkImageView>;
    auto createSampler(VkDevice device, VkPhysicalDeviceFeatures physicalFeatures, VkPhysicalDeviceProperties physicalProps,
        uint32_t mipLevels) -> Resource<VkSampler>;
    auto allocateImageMemory(VkDevice device, VkPhysicalDeviceMemoryProperties memProps, VkImage image) -> Resource<VkDeviceMemory>;
}
