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

    auto image = createImage(device, format, width, height, mipLevels, 1, 0, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    auto memory = allocateImageMemory(device, device.getPhysicalMemoryFeatures(), image);

    std::vector<VkBufferImageCopy> copyRegions;
    uint32_t offset = 0;

    for (uint32_t i = 0; i < mipLevels; i++)
    {
        const auto levelWidth = data.getWidth(i);
        const auto levelHeight = data.getHeight(i);
        const auto levelSize = data.getSize(i);

        VkBufferImageCopy bufferCopyRegion{};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = i;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = levelWidth;
        bufferCopyRegion.imageExtent.height = levelHeight;
        bufferCopyRegion.imageExtent.depth = 1;
        bufferCopyRegion.bufferOffset = offset;

        copyRegions.push_back(bufferCopyRegion);

        offset += levelSize;
    }

    auto stagingBuf = Buffer::createStaging(device, data.getSize(), data.getData());

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.layerCount = 1;

    auto copyCmdBuf = createCommandBuffer(device, device.getCommandPool());
    beginCommandBuffer(copyCmdBuf, true);

    setImageLayout(
        copyCmdBuf,
        image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        subresourceRange,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    vkCmdCopyBufferToImage(
        copyCmdBuf,
        stagingBuf,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        copyRegions.size(),
        copyRegions.data());

    auto finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    setImageLayout(
        copyCmdBuf,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        finalLayout,
        subresourceRange,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    vkEndCommandBuffer(copyCmdBuf);

    queueSubmit(device.getQueue(), 0, nullptr, 0, nullptr, 1, &copyCmdBuf);
    KL_VK_CHECK_RESULT(vkQueueWaitIdle(device.getQueue()));
    
    vkFreeCommandBuffers(device, device.getCommandPool(), 1, &copyCmdBuf);

    auto sampler = createSampler(device, device.getPhysicalFeatures(), device.getPhysicalProperties(), mipLevels);
    auto view = createImageView(device, format, VK_IMAGE_VIEW_TYPE_2D, mipLevels, 1, image, VK_IMAGE_ASPECT_COLOR_BIT);

    return Image{std::move(image), std::move(memory), std::move(view), std::move(sampler), finalLayout};
}

auto vk::Image::createCube(const Device &device, const ImageData &data) -> Image
{
    const auto mipLevels = data.getMipLevelCount();
    const auto width = data.getWidth(0, 0);
    const auto height = data.getHeight(0, 0);
    const auto format = toVulkanFormat(data.getFormat());

    auto image = createImage(device, format, width, height, mipLevels, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    auto memory = allocateImageMemory(device, device.getPhysicalMemoryFeatures(), image);

    std::vector<VkBufferImageCopy> copyRegions;
    uint32_t offset = 0;

    for (uint32_t face = 0; face < 6; face++)
    {
        for (uint32_t level = 0; level < mipLevels; level++)
        {
            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = level;
            bufferCopyRegion.imageSubresource.baseArrayLayer = face;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = data.getWidth(face, level);
            bufferCopyRegion.imageExtent.height = data.getHeight(face, level);
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = offset;

            copyRegions.push_back(bufferCopyRegion);

            offset += data.getSize(face, level);
        }
    }

    auto stagingBuf = Buffer::createStaging(device, data.getSize(), data.getData());

    VkImageSubresourceRange subresourceRange{};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = mipLevels;
	subresourceRange.layerCount = 6;

    auto copyCmdBuf = createCommandBuffer(device, device.getCommandPool());
    beginCommandBuffer(copyCmdBuf, true);

    setImageLayout(
        copyCmdBuf,
        image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        subresourceRange,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    vkCmdCopyBufferToImage(
        copyCmdBuf,
        stagingBuf,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        copyRegions.size(),
        copyRegions.data());

    auto imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    setImageLayout(
        copyCmdBuf,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        imageLayout,
        subresourceRange,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    vkEndCommandBuffer(copyCmdBuf);

    queueSubmit(device.getQueue(), 0, nullptr, 0, nullptr, 1, &copyCmdBuf);
    KL_VK_CHECK_RESULT(vkQueueWaitIdle(device.getQueue()));

    vkFreeCommandBuffers(device, device.getCommandPool(), 1, &copyCmdBuf);

    auto sampler = createSampler(device, device.getPhysicalFeatures(), device.getPhysicalProperties(), mipLevels);
    auto view = createImageView(device, format, VK_IMAGE_VIEW_TYPE_CUBE, mipLevels, 6, image, VK_IMAGE_ASPECT_COLOR_BIT);

    return Image{std::move(image), std::move(memory), std::move(view), std::move(sampler), imageLayout};
}

vk::Image::Image(Resource<VkImage> image, Resource<VkDeviceMemory> memory, Resource<VkImageView> view, Resource<VkSampler> sampler,
    VkImageLayout layout):
    image(std::move(image)),
    memory(std::move(memory)),
    view(std::move(view)),
    sampler(std::move(sampler)),
    layout(layout)
{
}
