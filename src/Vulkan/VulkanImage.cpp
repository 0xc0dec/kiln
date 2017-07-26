/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "../ImageData.h"
#include <vector>

static auto toVulkanFormat(ImageData::Format format) -> VkFormat
{
    switch (format)
    {
        case ImageData::Format::R8G8B8A8_UNORM:
            return VK_FORMAT_R8G8B8A8_UNORM;
        default:
            KL_PANIC("Unsupported texture format");
            return VK_FORMAT_UNDEFINED;
    }
}

static auto createImage(VkDevice device, VkFormat format, uint32_t width, uint32_t height, uint32_t mipLevels,
    uint32_t arrayLayers, VkImageCreateFlags createFlags, VkImageUsageFlags usageFlags) -> vk::Resource<VkImage>
{
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.mipLevels = mipLevels;
    imageCreateInfo.arrayLayers = arrayLayers;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = {width, height, 1};
    imageCreateInfo.usage = usageFlags;
    imageCreateInfo.flags = createFlags;

    vk::Resource<VkImage> image{device, vkDestroyImage};
    KL_VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, image.cleanRef()));

    return image;
}

static auto createSampler(VkDevice device, VkPhysicalDeviceFeatures physicalFeatures, VkPhysicalDeviceProperties physicalProps,
    uint32_t mipLevels) -> vk::Resource<VkSampler>
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = mipLevels;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    if (physicalFeatures.samplerAnisotropy)
    {
        samplerInfo.maxAnisotropy = physicalProps.limits.maxSamplerAnisotropy;
        samplerInfo.anisotropyEnable = VK_TRUE;
    }

    vk::Resource<VkSampler> sampler{device, vkDestroySampler};
    KL_VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, sampler.cleanRef()));

    return sampler;
}

static void setImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
    VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
{
    VkImageMemoryBarrier imageMemoryBarrier{};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (oldImageLayout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            // Image layout is undefined (or does not matter)
            // Only valid as initial layout
            // No flags required, listed only for completeness
            imageMemoryBarrier.srcAccessMask = 0;
            break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            // Image is preinitialized
            // Only valid as initial layout for linear images, preserves memory contents
            // Make sure host writes have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image is a color attachment
            // Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image is a depth/stencil attachment
            // Make sure any writes to the depth/stencil buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image is a transfer source 
            // Make sure any reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image is a transfer destination
            // Make sure any writes to the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image is read by a shader
            // Make sure any shader reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newImageLayout)
    {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image will be used as a transfer destination
            // Make sure any writes to the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image will be used as a transfer source
            // Make sure any reads from the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image will be used as a color attachment
            // Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image layout will be used as a depth/stencil attachment
            // Make sure any writes to depth/stencil buffer have been finished
            imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image will be read in a shader (sampler, input attachment)
            // Make sure any writes to the image have been finished
            if (imageMemoryBarrier.srcAccessMask == 0)
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            break;
    }

    vkCmdPipelineBarrier(cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

static auto allocateImageMemory(VkDevice device, VkPhysicalDeviceMemoryProperties memProps, VkImage image) -> vk::Resource<VkDeviceMemory>
{
    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(device, image, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = vk::findMemoryType(memProps, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vk::Resource<VkDeviceMemory> memory{device, vkFreeMemory};
    KL_VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, memory.cleanRef()));
    KL_VK_CHECK_RESULT(vkBindImageMemory(device, image, memory, 0));

    return memory;
}

auto vk::Image::create2D(const Device &device, const ImageData &data) -> Image
{
    const auto mipLevels = data.getMipLevelCount();
    const auto width = data.getWidth(0);
    const auto height = data.getHeight(0);
    const auto format = toVulkanFormat(data.getFormat());

    auto image = Image(device, width, height, mipLevels, 1, format,
        0,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_IMAGE_ASPECT_COLOR_BIT);
    image.uploadData(device, data);

    return image;
}

auto vk::Image::createCube(const Device &device, const ImageData &data) -> Image
{
    const auto mipLevels = data.getMipLevelCount();
    const auto width = data.getWidth(0, 0);
    const auto height = data.getHeight(0, 0);
    const auto format = toVulkanFormat(data.getFormat());

    auto image = Image(device, width, height, mipLevels, 6, format,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_VIEW_TYPE_CUBE,
        VK_IMAGE_ASPECT_COLOR_BIT);
    image.uploadData(device, data);

    return image;
}

vk::Image::Image(const Device &device, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layers, VkFormat format,
    VkImageCreateFlags createFlags, VkImageUsageFlags usageFlags, VkImageViewType viewType, VkImageAspectFlags aspectMask):
    mipLevels(mipLevels),
    layers(layers),
    aspectMask(aspectMask),
    width(width),
    height(height)
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

    VkImageSubresourceRange subresourceRange{};
	subresourceRange.aspectMask = aspectMask;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = mipLevels;
	subresourceRange.layerCount = layers;

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
