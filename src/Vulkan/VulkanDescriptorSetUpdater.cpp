/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "VulkanDescriptorSetUpdater.h"

vk::DescriptorSetUpdater::DescriptorSetUpdater(VkDevice device):
    device(device)
{
}

auto vk::DescriptorSetUpdater::forUniformBuffer(uint32_t binding, VkDescriptorSet set, VkBuffer buffer,
    VkDeviceSize offset, VkDeviceSize range) -> DescriptorSetUpdater&
{
    items.push_back({{buffer, offset, range}, {}, binding, set});
    return *this;
}

auto vk::DescriptorSetUpdater::forTexture(uint32_t binding, VkDescriptorSet set, VkImageView view,
    VkSampler sampler, VkImageLayout layout) -> DescriptorSetUpdater&
{
    items.push_back({{}, {sampler, view, layout}, binding, set});
    return *this;
}

void vk::DescriptorSetUpdater::updateSets()
{
    std::vector<VkWriteDescriptorSet> writes;

    for (const auto &item: items)
    {
        auto bufferInfo = item.buffer.buffer ? &item.buffer : nullptr;
        auto imageInfo = item.image.imageView ? &item.image : nullptr;
        auto type = bufferInfo ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = item.targetSet;
        write.dstBinding = item.binding;
        write.dstArrayElement = 0;
        write.descriptorType = type;
        write.descriptorCount = 1;
        write.pBufferInfo = bufferInfo;
        write.pImageInfo = imageInfo;
        write.pTexelBufferView = nullptr;
        writes.push_back(write);
    }

    vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
}
