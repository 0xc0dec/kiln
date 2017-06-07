/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include <vector>

static auto createImage(VkDevice device, VkFormat format, uint32_t width, uint32_t height,
    uint32_t mipLevels, uint32_t arrayLayers, VkImageCreateFlags flags) -> vk::Resource<VkImage>
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
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.flags = flags;

    vk::Resource<VkImage> image{device, vkDestroyImage};
    KL_VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, image.cleanRef()));

    return image;
}

static auto createView(VkDevice device, VkFormat format, VkImageViewType type, uint32_t mipLevels, uint32_t layers, VkImage image) -> vk::Resource<VkImageView>
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.viewType = type;
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.format = format;
    viewInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = layers;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.image = image;

    vk::Resource<VkImageView> view{device, vkDestroyImageView};
    KL_VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, view.cleanRef()));

    return view;
}

static auto createSampler(VkDevice device, vk::PhysicalDevice physicalDevice, uint32_t mipLevels) -> vk::Resource<VkSampler>
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevels);
    if (physicalDevice.features.samplerAnisotropy)
    {
        samplerInfo.maxAnisotropy = physicalDevice.properties.limits.maxSamplerAnisotropy;
        samplerInfo.anisotropyEnable = VK_TRUE;
    }
    else
    {
        samplerInfo.maxAnisotropy = 1.0;
        samplerInfo.anisotropyEnable = VK_FALSE;
    }
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    vk::Resource<VkSampler> sampler{device, vkDestroySampler};
    KL_VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, sampler.cleanRef()));

    return sampler;
}

static auto allocateImageMemory(VkDevice device, VkImage image, vk::PhysicalDevice physicalDevice) -> vk::Resource<VkDeviceMemory>
{
    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(device, image, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = vk::findMemoryType(physicalDevice.memoryProperties, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vk::Resource<VkDeviceMemory> memory{device, vkFreeMemory};
    KL_VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, memory.cleanRef()));
    KL_VK_CHECK_RESULT(vkBindImageMemory(device, image, memory, 0));

    return memory;
}

auto vk::Texture::create2D(VkDevice device, const PhysicalDevice &physicalDevice, VkFormat format,
    const gli::texture2d &data, VkCommandPool cmdPool, VkQueue queue) -> Texture
{
    const auto mipLevels = data.levels();
    const auto width = data[0].extent().x;
    const auto height = data[0].extent().y;

    auto image = createImage(device, format, width, height, mipLevels, 1, 0);
    auto memory = allocateImageMemory(device, image, physicalDevice);

    std::vector<VkBufferImageCopy> copyRegions;
    uint32_t offset = 0;

    for (uint32_t i = 0; i < mipLevels; i++)
    {
        VkBufferImageCopy bufferCopyRegion{};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = i;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(data[i].extent().x);
        bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(data[i].extent().y);
        bufferCopyRegion.imageExtent.depth = 1;
        bufferCopyRegion.bufferOffset = offset;

        copyRegions.push_back(bufferCopyRegion);

        offset += static_cast<uint32_t>(data[i].size());
    }

    auto stagingBuf = vk::Buffer(device, data.size(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        physicalDevice.memoryProperties);
    stagingBuf.update(data.data());

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

    auto sampler = createSampler(device, physicalDevice, mipLevels);
    auto view = createView(device, format, VK_IMAGE_VIEW_TYPE_2D, mipLevels, 1, image);

    return Texture{std::move(image), std::move(memory), std::move(view), std::move(sampler), imageLayout};
}

auto vk::Texture::createCube(VkDevice device, const PhysicalDevice &physicalDevice, VkFormat format,
    const gli::texture_cube &data, VkCommandPool cmdPool, VkQueue queue) -> Texture
{
    const auto mipLevels = data.levels();
    const auto width = data.extent().x;
    const auto height = data.extent().y;

    auto image = createImage(device, format, width, height, mipLevels, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
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

    auto stagingBuf = vk::Buffer(device, data.size(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        physicalDevice.memoryProperties);
    stagingBuf.update(data.data());

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

    auto sampler = createSampler(device, physicalDevice, mipLevels);
    auto view = createView(device, format, VK_IMAGE_VIEW_TYPE_CUBE, mipLevels, 6, image);

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
