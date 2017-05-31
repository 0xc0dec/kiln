/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vulkan.h"
#undef max // gli does not compile otherwise, probably because of Windows.h included earlier
#include <gli/gli.hpp>

namespace vk
{
    class VulkanBuffer;

    class Texture
    {
    public:
        static auto create2D(VkDevice device, const PhysicalDevice &physicalDevice, VkFormat format, VkBuffer dataBuffer,
            const gli::texture2d &info, VkCommandPool cmdPool, VkQueue queue) -> Texture;

        Texture(Resource<VkImage> image, Resource<VkDeviceMemory> memory, Resource<VkImageView> view, Resource<VkSampler> sampler);

        auto getLayout() -> VkImageLayout;
        auto getSampler() -> VkSampler;
        auto getView() -> VkImageView;

    private:
        Resource<VkImage> image;
        Resource<VkDeviceMemory> memory;
        Resource<VkImageView> view;
        Resource<VkSampler> sampler;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    inline auto Texture::getLayout() -> VkImageLayout
    {
        return layout;
    }

    inline auto Texture::getSampler() -> VkSampler
    {
        return sampler;
    }

    inline auto Texture::getView() -> VkImageView
    {
        return view;
    }
}