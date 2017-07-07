/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vulkan.h"
#include <functional>
#undef max // gli does not compile otherwise, probably because of Windows.h included earlier
#include <gli/gli.hpp>

namespace vk
{
    class VulkanBuffer;

    class Image
    {
    public:
        static auto create2D(VkDevice device, const PhysicalDevice &physicalDevice, VkCommandPool cmdPool, VkQueue queue,
            VkFormat format, uint32_t mipLevels, const void *data, uint32_t size,
            std::function<uint32_t(uint32_t mipLevel)> getLevelWidth,
            std::function<uint32_t(uint32_t mipLevel)> getLevelHeight,
            std::function<uint32_t(uint32_t mipLevel)> getLevelSize) -> Image;
        static auto createCube(VkDevice device, const PhysicalDevice &physicalDevice, VkFormat format,
            const gli::texture_cube &data, VkCommandPool cmdPool, VkQueue queue) -> Image;

        Image() {}
        Image(Resource<VkImage> image, Resource<VkDeviceMemory> memory, Resource<VkImageView> view,
            Resource<VkSampler> sampler, VkImageLayout layout);
        Image(const Image &other) = delete;
        Image(Image &&other) noexcept;

        auto operator=(Image other) noexcept -> Image&;

        auto getLayout() -> VkImageLayout;
        auto getSampler() -> VkSampler;
        auto getView() -> VkImageView;

    private:
        Resource<VkImage> image;
        Resource<VkDeviceMemory> memory;
        Resource<VkImageView> view;
        Resource<VkSampler> sampler;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

        void swap(Image &other) noexcept;
    };

    inline auto Image::getLayout() -> VkImageLayout
    {
        return layout;
    }

    inline auto Image::getSampler() -> VkSampler
    {
        return sampler;
    }

    inline auto Image::getView() -> VkImageView
    {
        return view;
    }
}