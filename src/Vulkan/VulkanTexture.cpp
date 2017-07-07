/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include <vector>

auto vk::Texture::create2D(VkDevice device, const PhysicalDevice &physicalDevice, VkCommandPool cmdPool, VkQueue queue,
    VkFormat format, uint32_t mipLevels, const void *data, uint32_t size,
    std::function<uint32_t(uint32_t mipLevel)> getLevelWidth,
    std::function<uint32_t(uint32_t mipLevel)> getLevelHeight,
    std::function<uint32_t(uint32_t mipLevel)> getLevelSize) -> Texture
{
    const auto width = getLevelWidth(0);
    const auto height = getLevelHeight(0);

    auto image = vk::createImage(device, format, width, height, mipLevels, 1, 0, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    auto memory = vk::allocateImageMemory(device, image, physicalDevice);

    std::vector<VkBufferImageCopy> copyRegions;
    uint32_t offset = 0;

    for (uint32_t i = 0; i < mipLevels; i++)
    {
        const auto levelWidth = getLevelWidth(i);
        const auto levelHeight = getLevelHeight(i);
        const auto levelSize = getLevelSize(i);

        VkBufferImageCopy bufferCopyRegion{};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = i;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(levelWidth);
        bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(levelHeight);
        bufferCopyRegion.imageExtent.depth = 1;
        bufferCopyRegion.bufferOffset = offset;

        copyRegions.push_back(bufferCopyRegion);

        offset += static_cast<uint32_t>(levelSize);
    }

    auto stagingBuf = vk::Buffer::createStaging(device, size, physicalDevice, data);

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.layerCount = 1;

    auto copyCmdBuf = vk::createCommandBuffer(device, cmdPool);
    vk::beginCommandBuffer(copyCmdBuf, true);

    vk::setImageLayout(
        copyCmdBuf,
        image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        subresourceRange,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    vkCmdCopyBufferToImage(
        copyCmdBuf,
        stagingBuf.getHandle(),
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(copyRegions.size()),
        copyRegions.data());

    auto finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vk::setImageLayout(
        copyCmdBuf,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        finalLayout,
        subresourceRange,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    vkEndCommandBuffer(copyCmdBuf);

    vk::queueSubmit(queue, 0, nullptr, 0, nullptr, 1, &copyCmdBuf);
    KL_VK_CHECK_RESULT(vkQueueWaitIdle(queue));
    
    vkFreeCommandBuffers(device, cmdPool, 1, &copyCmdBuf);

    auto sampler = createSampler(device, physicalDevice, mipLevels);
    auto view = vk::createImageView(device, format, VK_IMAGE_VIEW_TYPE_2D, mipLevels, 1, image, VK_IMAGE_ASPECT_COLOR_BIT);

    return Texture{std::move(image), std::move(memory), std::move(view), std::move(sampler), finalLayout};
}

auto vk::Texture::createCube(VkDevice device, const PhysicalDevice &physicalDevice, VkFormat format,
    const gli::texture_cube &data, VkCommandPool cmdPool, VkQueue queue) -> Texture
{
    const auto mipLevels = data.levels();
    const auto width = data.extent().x;
    const auto height = data.extent().y;

    auto image = vk::createImage(device, format, width, height, mipLevels, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    auto memory = allocateImageMemory(device, image, physicalDevice);

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
            bufferCopyRegion.imageExtent.width = data[face][level].extent().x;
            bufferCopyRegion.imageExtent.height = data[face][level].extent().y;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = offset;

            copyRegions.push_back(bufferCopyRegion);

            offset += data[face][level].size();
        }
    }

    auto stagingBuf = vk::Buffer::createStaging(device, data.size(), physicalDevice, data.data());

    VkImageSubresourceRange subresourceRange{};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = mipLevels;
	subresourceRange.layerCount = 6;

    auto copyCmdBuf = vk::createCommandBuffer(device, cmdPool);
    vk::beginCommandBuffer(copyCmdBuf, true);

    vk::setImageLayout(
        copyCmdBuf,
        image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        subresourceRange,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    vkCmdCopyBufferToImage(
        copyCmdBuf,
        stagingBuf.getHandle(),
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(copyRegions.size()),
        copyRegions.data());

    auto imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vk::setImageLayout(
        copyCmdBuf,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        imageLayout,
        subresourceRange,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    vkEndCommandBuffer(copyCmdBuf);

    vk::queueSubmit(queue, 0, nullptr, 0, nullptr, 1, &copyCmdBuf);
    KL_VK_CHECK_RESULT(vkQueueWaitIdle(queue));

    vkFreeCommandBuffers(device, cmdPool, 1, &copyCmdBuf);

    auto sampler = createSampler(device, physicalDevice, mipLevels);
    auto view = vk::createImageView(device, format, VK_IMAGE_VIEW_TYPE_CUBE, mipLevels, 6, image, VK_IMAGE_ASPECT_COLOR_BIT);

    return Texture{std::move(image), std::move(memory), std::move(view), std::move(sampler), imageLayout};
}

vk::Texture::Texture(Resource<VkImage> image, Resource<VkDeviceMemory> memory, Resource<VkImageView> view, Resource<VkSampler> sampler,
    VkImageLayout layout):
    image(std::move(image)),
    memory(std::move(memory)),
    view(std::move(view)),
    sampler(std::move(sampler)),
    layout(layout)
{
}

vk::Texture::Texture(Texture &&other) noexcept
{
    swap(other);
}

auto vk::Texture::operator=(Texture other) noexcept -> Texture&

{
    swap(other);
    return *this;
}

void vk::Texture::swap(Texture &other) noexcept
{
    std::swap(image, other.image);
    std::swap(memory, other.memory);
    std::swap(view, other.view);
    std::swap(sampler, other.sampler);
    std::swap(layout, other.layout);
}
