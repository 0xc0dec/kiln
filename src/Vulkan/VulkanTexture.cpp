/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "VulkanTexture.h"
#include <vector>

auto vk::Texture::create2D(VkDevice device, const PhysicalDevice &physicalDevice, VkFormat format, VkBuffer dataBuffer,
    const gli::texture2d &info, VkCommandPool cmdPool, VkQueue queue) -> Texture
{
    const auto mipLevels = info.levels();
    const auto width = info[0].extent().x;
    const auto height = info[0].extent().y;

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.mipLevels = mipLevels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    Resource<VkImage> image{device, vkDestroyImage};
    KL_VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, image.cleanRef()));

    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(device, image, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = vk::findMemoryType(physicalDevice.memoryProperties, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Resource<VkDeviceMemory> memory{device, vkFreeMemory};
    KL_VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, memory.cleanRef()));
    KL_VK_CHECK_RESULT(vkBindImageMemory(device, image, memory, 0));

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.layerCount = 1;

    // TODO use some helper funcs
    auto copyCmdBuf = vk::createCommandBuffer(device, cmdPool);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(copyCmdBuf, &beginInfo);

    vk::setImageLayout(
        copyCmdBuf,
        image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        subresourceRange,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    std::vector<VkBufferImageCopy> bufferCopyRegions;
    uint32_t offset = 0;

    for (uint32_t i = 0; i < mipLevels; i++)
    {
        VkBufferImageCopy bufferCopyRegion{};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = i;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(info[i].extent().x);
        bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(info[i].extent().y);
        bufferCopyRegion.imageExtent.depth = 1;
        bufferCopyRegion.bufferOffset = offset;

        bufferCopyRegions.push_back(bufferCopyRegion);

        offset += static_cast<uint32_t>(info[i].size());
    }

    vkCmdCopyBufferToImage(
        copyCmdBuf,
        dataBuffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(bufferCopyRegions.size()),
        bufferCopyRegions.data());

    // Change texture image layout to shader read after all mip levels have been copied
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

    // Sampler

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
	// Set max level-of-detail to mip level count of the texture
	samplerInfo.maxLod = static_cast<float>(mipLevels);
	// Enable anisotropic filtering
	// This feature is optional, so we must check if it's supported on the device
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

    Resource<VkSampler> sampler{device, vkDestroySampler};
	KL_VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, sampler.cleanRef()));

    VkImageViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.format = format;
	viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.image = image;

    Resource<VkImageView> view{device, vkDestroyImageView};
	KL_VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, view.cleanRef()));

    return Texture{std::move(image), std::move(memory), std::move(view), std::move(sampler)};
}

vk::Texture::Texture(Resource<VkImage> image, Resource<VkDeviceMemory> memory, Resource<VkImageView> view, Resource<VkSampler> sampler):
    image(std::move(image)),
    memory(std::move(memory)),
    view(std::move(view)),
    sampler(std::move(sampler))
{
}
