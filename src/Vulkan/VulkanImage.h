/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vulkan.h"

class ImageData;

namespace vk
{
    class Image
    {
    public:
        static auto create2D(VkDevice device, const PhysicalDevice &physicalDevice, VkCommandPool cmdPool, VkQueue queue, ImageData *data) -> Image;
        static auto createCube(VkDevice device, const PhysicalDevice &physicalDevice, VkCommandPool cmdPool, VkQueue queue, ImageData *data) -> Image;

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