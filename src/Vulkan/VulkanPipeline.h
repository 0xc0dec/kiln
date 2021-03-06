/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vulkan.h"
#include <vector>

class VertexFormat;

namespace vk
{
    class PipelineConfig
    {
    public:
        PipelineConfig(VkShaderModule vertexShader, VkShaderModule fragmentShader);
        ~PipelineConfig() {}

        auto withVertexAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset) -> PipelineConfig&;
        auto withVertexBinding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate) -> PipelineConfig&;
        auto withVertexFormat(const VertexFormat &format) -> PipelineConfig&;

        auto withDescriptorSetLayout(VkDescriptorSetLayout layout) -> PipelineConfig&;

        auto withFrontFace(VkFrontFace frontFace) -> PipelineConfig&;
        auto withCullMode(VkCullModeFlags cullFlags) -> PipelineConfig&;

        auto withDepthTest(bool write, bool test) -> PipelineConfig&;

        auto withBlend(bool enabled, VkBlendFactor srcColorFactor, VkBlendFactor dstColorFactor,
            VkBlendFactor srcAlphaFactor, VkBlendFactor dstAlphaFactor) -> PipelineConfig&;

        auto withTopology(VkPrimitiveTopology topology) -> PipelineConfig&
        {
            this->topology = topology;
            return *this;
        }

    private:
        friend class Pipeline;

        VkShaderModule vertexShader;
        VkShaderModule fragmentShader;
        VkPipelineRasterizationStateCreateInfo rasterStateInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo;
        VkPipelineColorBlendAttachmentState blendAttachmentState;

        std::vector<VkVertexInputAttributeDescription> vertexAttrs;
        std::vector<VkVertexInputBindingDescription> vertexBindings;
        std::vector<VkDescriptorSetLayout> descSetLayouts;

        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    };

    class Pipeline
    {
    public:
        Pipeline() {}
        Pipeline(VkDevice device, VkRenderPass renderPass, const PipelineConfig &config);
        Pipeline(const Pipeline &other) = delete;
        Pipeline(Pipeline &&other) = default;
        ~Pipeline() {}

        auto operator=(const Pipeline &other) -> Pipeline& = delete;
        auto operator=(Pipeline &&other) -> Pipeline& = default;

        operator VkPipeline() { return pipeline; }

        auto getHandle() const -> VkPipeline { return pipeline; }
        auto getLayout() const -> VkPipelineLayout { return layout; }

    private:
        Resource<VkPipeline> pipeline;
        Resource<VkPipelineLayout> layout;
    };
}
