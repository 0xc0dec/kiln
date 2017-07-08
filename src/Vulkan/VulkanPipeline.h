/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vulkan.h"
#include <vector>

namespace vk
{
    class Pipeline
    {
    public:
        Pipeline() {}
        // TODO why not pass pipeline/layout as const refs?
        Pipeline(VkDevice device, VkRenderPass renderPass, Resource<VkPipeline> pipeline, Resource<VkPipelineLayout> layout);
        Pipeline(const Pipeline &other) = delete;
        Pipeline(Pipeline &&other) = default;
        ~Pipeline() {}

        auto operator=(const Pipeline &other) -> Pipeline& = delete;
        auto operator=(Pipeline &&other) -> Pipeline& = default;

        operator VkPipeline()
        {
            return pipeline;
        }

        VkPipeline getHandle() const;
        VkPipelineLayout getLayout() const;

    private:
        VkDevice device = nullptr;
        VkRenderPass renderPass = nullptr;
        Resource<VkPipeline> pipeline;
        Resource<VkPipelineLayout> layout;
    };

    class PipelineBuilder
    {
    public:
        PipelineBuilder(VkDevice device, VkRenderPass renderPass, VkShaderModule vertexShader, VkShaderModule fragmentShader);
        ~PipelineBuilder() {}

        auto withTopology(VkPrimitiveTopology topology) -> PipelineBuilder&;

        auto withVertexAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset) -> PipelineBuilder&;
        auto withVertexBinding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate) -> PipelineBuilder&;

        auto withDescriptorSetLayout(VkDescriptorSetLayout layout) -> PipelineBuilder&;

        auto withFrontFace(VkFrontFace frontFace) -> PipelineBuilder&;
        auto withCullMode(VkCullModeFlags cullFlags) -> PipelineBuilder&;

        auto withDepthTest(bool write, bool test) -> PipelineBuilder&;

        auto build() -> Pipeline;

    private:
        VkDevice device = nullptr;
        VkRenderPass renderPass = nullptr;

        VkShaderModule vertexShader;
        VkShaderModule fragmentShader;
        VkPipelineShaderStageCreateInfo vertexShaderStageInfo;
        VkPipelineShaderStageCreateInfo fragmentShaderStageInfo;
        VkPipelineRasterizationStateCreateInfo rasterState;
        VkPipelineDepthStencilStateCreateInfo depthStencilState;

        std::vector<VkVertexInputAttributeDescription> vertexAttrs;
        std::vector<VkVertexInputBindingDescription> vertexBindings;
        std::vector<VkDescriptorSetLayout> descSetLayouts;

        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    };
    
    inline VkPipeline Pipeline::getHandle() const
    {
        return pipeline;
    }

    inline VkPipelineLayout Pipeline::getLayout() const
    {
        return layout;
    }

    inline auto PipelineBuilder::withTopology(VkPrimitiveTopology topology) -> PipelineBuilder&
    {
        this->topology = topology;
        return *this;
    }
}
