/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "../ImageData.h"
#include <vector>

auto toVulkanFormat(ImageData::Format format) -> VkFormat
{
    switch (format)
    {
        case ImageData::Format::R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
        default:
            KL_PANIC("Unsupported texture format");
            return VK_FORMAT_UNDEFINED;
    }
}

auto vk::Image::create2D(const Device &device, const ImageData &data) -> Image
{
    const auto mipLevels = data.getMipLevelCount();
    const auto width = data.getWidth(0);
    const auto height = data.getHeight(0);
    const auto format = toVulkanFormat(data.getFormat());

    auto image = Image(device, width, height, mipLevels, 1, format, 0, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);
    image.uploadData(device, data);

    return image;
}

auto vk::Image::createCube(const Device &device, const ImageData &data) -> Image
{
    const auto mipLevels = data.getMipLevelCount();
    const auto width = data.getWidth(0, 0);
    const auto height = data.getHeight(0, 0);
    const auto format = toVulkanFormat(data.getFormat());

    auto image = Image(device, width, height, mipLevels, 6, format, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_ASPECT_COLOR_BIT);
    image.uploadData(device, data);

    return image;
}

vk::Image::Image(const Device &device, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layers, VkFormat format,
    VkImageCreateFlags createFlags, VkImageUsageFlags usageFlags, VkImageViewType viewType, VkImageAspectFlags aspectMask):
    mipLevels(mipLevels),
    layers(layers),
    aspectMask(aspectMask)
{
    auto image = createImage(device, format, width, height, mipLevels, layers, createFlags, usageFlags);
    auto memory = allocateImageMemory(device, device.getPhysicalMemoryFeatures(), image);
    auto sampler = createSampler(device, device.getPhysicalFeatures(), device.getPhysicalProperties(), mipLevels);
    auto view = createImageView(device, format, viewType, mipLevels, layers, image, aspectMask);
    this->image = std::move(image);
    this->memory = std::move(memory);
    this->sampler = std::move(sampler);
    this->view = std::move(view);
}

void vk::Image::uploadData(const Device &device, const ImageData &data)
{
    VkImageSubresourceRange subresourceRange{};
	subresourceRange.aspectMask = aspectMask;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = mipLevels;
	subresourceRange.layerCount = layers;

    uint32_t offset = 0;
    std::vector<VkBufferImageCopy> copyRegions;
    for (uint32_t layer = 0; layer < layers; layer++)
    {
        for (uint32_t level = 0; level < mipLevels; level++)
        {
            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = level;
            bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = data.getWidth(layer, level);
            bufferCopyRegion.imageExtent.height = data.getHeight(layer, level);
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = offset;

            copyRegions.push_back(bufferCopyRegion);

            offset += data.getSize(layer, level);
        }
    }

    auto srcBuf = Buffer::createStaging(device, data.getSize(), data.getData());

    auto cmdBuf = createCommandBuffer(device, device.getCommandPool());
    beginCommandBuffer(cmdBuf, true);

    setImageLayout(
        cmdBuf,
        image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        subresourceRange,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    vkCmdCopyBufferToImage(
        cmdBuf,
        srcBuf,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        copyRegions.size(),
        copyRegions.data());

    auto imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    setImageLayout(
        cmdBuf,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        imageLayout,
        subresourceRange,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    vkEndCommandBuffer(cmdBuf);

    queueSubmit(device.getQueue(), 0, nullptr, 0, nullptr, 1, &cmdBuf);
    KL_VK_CHECK_RESULT(vkQueueWaitIdle(device.getQueue()));
}
