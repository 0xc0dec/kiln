/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vulkan.h"
#include <vector>

namespace vk
{
    class Swapchain
    {
    public:
        Swapchain() {}
        Swapchain(const Swapchain &other) = delete;
        Swapchain(Swapchain &&other) = default;
        Swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkRenderPass renderPass,
            VkImageView depthStencilView, uint32_t width, uint32_t height, bool vsync, VkFormat colorFormat, VkColorSpaceKHR colorSpace);
        ~Swapchain() {}

        auto operator=(const Swapchain &other) -> Swapchain& = delete;
        auto operator=(Swapchain &&other) -> Swapchain& = default;

        operator VkSwapchainKHR() { return swapchain; }
        operator VkSwapchainKHR() const { return swapchain; }

        auto getHandle() const -> VkSwapchainKHR;

        auto getNextStep(VkSemaphore semaphore) const -> uint32_t;
        auto getStepCount() const -> uint32_t;
        auto getFramebuffer(uint32_t idx) const -> VkFramebuffer;
        auto getImageView(uint32_t idx) -> VkImageView;

    private:
        struct Step
        {
            VkImage image;
            Resource<VkImageView> imageView;
            Resource<VkFramebuffer> framebuffer;
        };

        VkDevice device = nullptr;
        Resource<VkSwapchainKHR> swapchain;
        std::vector<Step> steps;
    };

    inline auto Swapchain::getHandle() const -> VkSwapchainKHR
    {
        return swapchain;
    }

    inline auto Swapchain::getStepCount() const -> uint32_t
    {
        return steps.size();
    }

    inline auto Swapchain::getFramebuffer(uint32_t idx) const -> VkFramebuffer
    {
        return steps[idx].framebuffer;
    }

    inline auto Swapchain::getImageView(uint32_t idx) -> VkImageView
    {
        return steps[idx].imageView;
    }
}