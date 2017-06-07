/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "VulkanDescriptorSetUpdater.h"

auto vk::DescriptorSetUpdater::forUniformBuffer(uint32_t binding, VkDescriptorSet set, VkBuffer buffer,
    VkDeviceSize offset, VkDeviceSize range) -> DescriptorSetUpdater&
{
    buffers.push_back({{buffer, offset, range}, binding, set});
    return *this;
}

auto vk::DescriptorSetUpdater::forTexture(uint32_t binding, VkDescriptorSet set, VkImageView view,
    VkSampler sampler, VkImageLayout layout) -> DescriptorSetUpdater&
{
    textures.push_back({{sampler, view, layout}, binding, set});
    return *this;
}

void vk::DescriptorSetUpdater::updateSets(VkDevice device)
{
    std::vector<VkWriteDescriptorSet> writes;

    for (const auto &buffer: buffers)
    {
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = buffer.targetSet;
        write.dstBinding = buffer.binding;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &buffer.info;
        write.pImageInfo = nullptr;
        write.pTexelBufferView = nullptr;
        writes.push_back(write);
    }

    for (const auto &texture: textures)
    {
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = texture.targetSet;
        write.dstBinding = texture.binding;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pBufferInfo = nullptr;
        write.pImageInfo = &texture.info;
        write.pTexelBufferView = nullptr;
        writes.push_back(write);
    }

    vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
}
