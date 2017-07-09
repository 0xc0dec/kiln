/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vulkan.h"
#include <vector>

namespace vk
{
    class RenderPass
    {
    public:
        RenderPass() {}
        RenderPass(VkDevice device, Resource<VkRenderPass> pass);
        RenderPass(const RenderPass &other) = delete;
        RenderPass(RenderPass &&other) = default;
        ~RenderPass() {}

        auto operator=(const RenderPass &other) -> RenderPass& = delete;
        auto operator=(RenderPass &&other) -> RenderPass& = default;

        void setClear(bool clearColor, bool clearDepthStencil, VkClearColorValue color, VkClearDepthStencilValue depthStencil);

        void begin(VkCommandBuffer cmdBuf, VkFramebuffer framebuffer, uint32_t canvasWidth, uint32_t canvasHeight);
        void end(VkCommandBuffer cmdBuf);

        operator VkRenderPass() { return pass; }

    private:
        VkDevice device = nullptr;
        Resource<VkRenderPass> pass;
        std::vector<VkClearValue> clearValues;
    };

    class RenderPassBuilder
    {
    public:
        explicit RenderPassBuilder(VkDevice device);

        auto withColorAttachment(VkFormat colorFormat, VkImageLayout finalLayout) -> RenderPassBuilder&;
        auto withDepthAttachment(VkFormat depthFormat) -> RenderPassBuilder&;

        auto build() -> RenderPass;

    private:
        VkDevice device = nullptr;
        std::vector<VkAttachmentDescription> attachments;
        std::vector<VkAttachmentReference> colorAttachmentRefs;
        VkAttachmentReference depthAttachmentRef;
    };
}