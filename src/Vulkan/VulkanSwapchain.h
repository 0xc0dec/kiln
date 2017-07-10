/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vulkan.h"
#include "VulkanImage.h"
#include <vector>
#include <functional>

namespace vk
{
    class Device;

    class Swapchain
    {
    public:
        Swapchain() {}
        Swapchain(const Device &device, VkRenderPass renderPass, uint32_t width, uint32_t height, bool vsync);
        Swapchain(const Swapchain &other) = delete;
        Swapchain(Swapchain &&other) = default;
        ~Swapchain() {}

        auto operator=(const Swapchain &other) -> Swapchain& = delete;
        auto operator=(Swapchain &&other) -> Swapchain& = default;

        operator VkSwapchainKHR() { return swapchain; }
        operator VkSwapchainKHR() const { return swapchain; }

        auto getHandle() const -> VkSwapchainKHR { return swapchain; }
        auto getStepCount() const -> uint32_t { return steps.size(); }
        auto getFramebuffer(uint32_t idx) const -> VkFramebuffer { return steps[idx].framebuffer; }
        auto getImageView(uint32_t idx) -> VkImageView { return steps[idx].imageView; }
        auto getPresentCompleteSem() const -> VkSemaphore { return presentCompleteSem; }

        auto getNextStep() const -> uint32_t;

        void recordRenderCommands(std::function<void(uint32_t, VkCommandBuffer)> issueCommands);

        void presentNext(VkQueue queue, uint32_t step, uint32_t waitSemaphoreCount, const VkSemaphore *waitSemaphores);

    private:
        struct Step
        {
            VkImage image;
            Resource<VkImageView> imageView;
            Resource<VkFramebuffer> framebuffer;
            Resource<VkCommandBuffer> cmdBuffer;
        };

        VkDevice device = nullptr;
        Resource<VkSwapchainKHR> swapchain;
        Image depthStencil;
        std::vector<Step> steps;
        Resource<VkSemaphore> presentCompleteSem;
        Resource<VkSemaphore> renderCompleteSem;
    };
}