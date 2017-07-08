/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "VulkanPipeline.h"

vk::Pipeline::Pipeline(VkDevice device, VkRenderPass renderPass, Resource<VkPipeline> pipeline, Resource<VkPipelineLayout> layout):
    device(device),
    renderPass(renderPass),
    pipeline(std::move(pipeline)),
    layout(std::move(layout))
{
}

vk::PipelineBuilder::PipelineBuilder(VkDevice device, VkRenderPass renderPass, VkShaderModule vertexShader, VkShaderModule fragmentShader):
    device(device),
    renderPass(renderPass),
    vertexShader(vertexShader),
    fragmentShader(fragmentShader),
    rasterState{},
    depthStencilState{}
{
    vertexShaderStageInfo = createShaderStageInfo(true, vertexShader, "main");
    fragmentShaderStageInfo = createShaderStageInfo(false, fragmentShader, "main");

    rasterState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterState.pNext = nullptr;
    rasterState.flags = 0;
    rasterState.depthClampEnable = false;
    rasterState.rasterizerDiscardEnable = false;
    rasterState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterState.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterState.depthBiasEnable = false;
    rasterState.depthBiasClamp = 0;
    rasterState.depthBiasConstantFactor = 0;
    rasterState.depthBiasSlopeFactor = 0;
    rasterState.lineWidth = 1;

	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.flags = 0;
	depthStencilState.depthTestEnable = true;
	depthStencilState.depthWriteEnable = true;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilState.front = depthStencilState.back;
}

auto vk::PipelineBuilder::withVertexAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset) -> PipelineBuilder&
{
    if (location >= vertexAttrs.size())
        vertexAttrs.resize(location + 1);
    vertexAttrs[location].location = location;
    vertexAttrs[location].binding = binding;
    vertexAttrs[location].format = format;
    vertexAttrs[location].offset = offset;
    return *this;
}

auto vk::PipelineBuilder::withVertexBinding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate) -> PipelineBuilder &
{
    if (binding >= vertexBindings.size())
        vertexBindings.resize(binding + 1);
    vertexBindings[binding].binding = binding;
    vertexBindings[binding].stride = stride;
    vertexBindings[binding].inputRate = inputRate;
    return *this;
}

auto vk::PipelineBuilder::withDescriptorSetLayout(VkDescriptorSetLayout layout) -> PipelineBuilder&
{
    descSetLayouts.push_back(layout);
    return *this;
}

auto vk::PipelineBuilder::withFrontFace(VkFrontFace frontFace) -> PipelineBuilder&
{
    rasterState.frontFace = frontFace;
    return *this;
}

auto vk::PipelineBuilder::withCullMode(VkCullModeFlags cullFlags) -> PipelineBuilder&
{
    rasterState.cullMode = cullFlags;
    return *this;
}

auto vk::PipelineBuilder::withDepthTest(bool write, bool test) -> PipelineBuilder &
{
    depthStencilState.depthWriteEnable = write;
    depthStencilState.depthTestEnable = test;
    return *this;
}

auto vk::PipelineBuilder::build() -> Pipeline
{
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = nullptr;
    layoutInfo.flags = 0;
    layoutInfo.setLayoutCount = descSetLayouts.size();
    layoutInfo.pSetLayouts = descSetLayouts.data();
    layoutInfo.pushConstantRangeCount = 0;
    layoutInfo.pPushConstantRanges = nullptr;

    Resource<VkPipelineLayout> layout{device, vkDestroyPipelineLayout};
    KL_VK_CHECK_RESULT(vkCreatePipelineLayout(device, &layoutInfo, nullptr, layout.cleanRef()));

    VkPipelineMultisampleStateCreateInfo multisampleState{};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.pNext = nullptr;
    multisampleState.flags = 0;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleState.sampleShadingEnable = false;
    multisampleState.minSampleShading = 0;
    multisampleState.pSampleMask = nullptr;
    multisampleState.alphaToCoverageEnable = false;
    multisampleState.alphaToOneEnable = false;

    VkPipelineColorBlendAttachmentState blendAttachmentState{};
    blendAttachmentState.blendEnable = VK_FALSE;
    blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendState{};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.pNext = nullptr;
    colorBlendState.flags = 0;
    colorBlendState.logicOpEnable = VK_FALSE;
    colorBlendState.logicOp = VK_LOGIC_OP_COPY;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &blendAttachmentState;
    colorBlendState.blendConstants[0] = 0;
    colorBlendState.blendConstants[1] = 0;
    colorBlendState.blendConstants[2] = 0;
    colorBlendState.blendConstants[3] = 0;

    std::vector<VkPipelineShaderStageCreateInfo> shaderStageStates{vertexShaderStageInfo, fragmentShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputState{};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.pNext = nullptr;
    vertexInputState.flags = 0;
    vertexInputState.vertexBindingDescriptionCount = vertexBindings.size();
    vertexInputState.pVertexBindingDescriptions = vertexBindings.data();
    vertexInputState.vertexAttributeDescriptionCount = vertexAttrs.size();
    vertexInputState.pVertexAttributeDescriptions = vertexAttrs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.pNext = nullptr;
    inputAssemblyState.flags = 0;
    inputAssemblyState.topology = topology;
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;
    viewportState.flags = 0;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr;
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;

    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext = nullptr;
    dynamicState.flags = 0;
    dynamicState.dynamicStateCount = dynamicStates.size();
    dynamicState.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = 0;
    pipelineInfo.stageCount = shaderStageStates.size();
    pipelineInfo.pStages = shaderStageStates.data();
    pipelineInfo.pVertexInputState = &vertexInputState;
    pipelineInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineInfo.pTessellationState = nullptr;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterState;
    pipelineInfo.pMultisampleState = &multisampleState;
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.pColorBlendState = &colorBlendState;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    Resource<VkPipeline> pipeline{device, vkDestroyPipeline};
    KL_VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, pipeline.cleanRef()));

    return Pipeline(device, renderPass, std::move(pipeline), std::move(layout));
}
