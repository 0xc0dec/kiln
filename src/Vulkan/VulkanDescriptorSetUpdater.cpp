/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "VulkanDescriptorSetUpdater.h"

auto vk::DescriptorSetUpdater::forUniformBuffer(uint32_t binding, VkDescriptorSet set, VkBuffer buffer,
    VkDeviceSize offset, VkDeviceSize range) -> DescriptorSetUpdater&
{
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = offset;
    bufferInfo.range = range;
    bufferInfos.push_back(bufferInfo);

    VkWriteDescriptorSet uboDescriptorWrite{};
    uboDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    uboDescriptorWrite.dstSet = set;
    uboDescriptorWrite.dstBinding = binding;
    uboDescriptorWrite.dstArrayElement = 0;
    uboDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboDescriptorWrite.descriptorCount = 1;
    uboDescriptorWrite.pBufferInfo = &bufferInfos[bufferInfos.size() - 1];
    uboDescriptorWrite.pImageInfo = nullptr;
    uboDescriptorWrite.pTexelBufferView = nullptr;
    writes.push_back(uboDescriptorWrite);

    return *this;
}

auto vk::DescriptorSetUpdater::forTexture(uint32_t binding, VkDescriptorSet set, VkImageView view,
    VkSampler sampler, VkImageLayout layout) -> DescriptorSetUpdater&
{
    VkDescriptorImageInfo imageInfo{};
	imageInfo.imageView = view;
	imageInfo.sampler = sampler;
	imageInfo.imageLayout = layout;
    imageInfos.push_back(imageInfo);

    VkWriteDescriptorSet textureDescriptorWrite{};
    textureDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    textureDescriptorWrite.dstSet = set;
    textureDescriptorWrite.dstBinding = binding;
    textureDescriptorWrite.dstArrayElement = 0;
    textureDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureDescriptorWrite.descriptorCount = 1;
    textureDescriptorWrite.pBufferInfo = nullptr;
    textureDescriptorWrite.pImageInfo = &imageInfos[imageInfos.size() - 1];;
    textureDescriptorWrite.pTexelBufferView = nullptr;
    writes.push_back(textureDescriptorWrite);

    return *this;
}

void vk::DescriptorSetUpdater::updateSets(VkDevice device)
{
    vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
}
