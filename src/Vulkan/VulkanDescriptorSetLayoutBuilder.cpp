/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "VulkanDescriptorSetLayoutBuilder.h"

vk::DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder(VkDevice device):
    device(device)
{
}

auto vk::DescriptorSetLayoutBuilder::withBinding(uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount,
    VkShaderStageFlagBits stageFlags) -> DescriptorSetLayoutBuilder&
{
    if (binding >= bindings.size())
        bindings.resize(binding + 1);

    bindings[binding].binding = binding;
    bindings[binding].descriptorType = descriptorType;
    bindings[binding].descriptorCount = descriptorCount;
    bindings[binding].stageFlags = stageFlags;
    bindings[binding].pImmutableSamplers = nullptr;

    return *this;
}

auto vk::DescriptorSetLayoutBuilder::build() -> Resource<VkDescriptorSetLayout>
{
    VkDescriptorSetLayoutCreateInfo layoutInfo {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    Resource<VkDescriptorSetLayout> result{device, vkDestroyDescriptorSetLayout};
    KL_VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, result.cleanRef()));
    
    return result;
}