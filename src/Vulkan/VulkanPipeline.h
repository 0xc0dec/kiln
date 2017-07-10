/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vulkan.h"
#include <vector>

namespace vk
{
    class PipelineConfig
    {
    public:
        PipelineConfig(VkShaderModule vertexShader, VkShaderModule fragmentShader);
        ~PipelineConfig() {}

        auto withTopology(VkPrimitiveTopology topology) -> PipelineConfig&;

        auto withVertexAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset) -> PipelineConfig&;
        auto withVertexBinding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate) -> PipelineConfig&;

        auto withDescriptorSetLayout(VkDescriptorSetLayout layout) -> PipelineConfig&;

        auto withFrontFace(VkFrontFace frontFace) -> PipelineConfig&;
        auto withCullMode(VkCullModeFlags cullFlags) -> PipelineConfig&;

        auto withDepthTest(bool write, bool test) -> PipelineConfig&;

    private:
        friend class Pipeline;

        VkShaderModule vertexShader;
        VkShaderModule fragmentShader;
        VkPipelineRasterizationStateCreateInfo rasterStateInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo;

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

        VkPipeline getHandle() const;
        VkPipelineLayout getLayout() const;

    private:
        Resource<VkPipeline> pipeline;
        Resource<VkPipelineLayout> layout;
    };

    inline VkPipeline Pipeline::getHandle() const
    {
        return pipeline;
    }

    inline VkPipelineLayout Pipeline::getLayout() const
    {
        return layout;
    }

    inline auto PipelineConfig::withTopology(VkPrimitiveTopology topology) -> PipelineConfig&
    {
        this->topology = topology;
        return *this;
    }
}
