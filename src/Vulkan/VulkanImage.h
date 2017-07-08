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
        static auto create2D(VkDevice device, const PhysicalDevice &physicalDevice, VkCommandPool cmdPool, VkQueue queue, const ImageData &data) -> Image;
        static auto createCube(VkDevice device, const PhysicalDevice &physicalDevice, VkCommandPool cmdPool, VkQueue queue, const ImageData &data) -> Image;

        Image() {}
        Image(const Image &other) = delete;
        Image(Image &&other) = default;

        auto operator=(const Image &other) -> Image& = delete;
        auto operator=(Image &&other) -> Image& = default;

        auto getLayout() const -> VkImageLayout;
        auto getSampler() const -> VkSampler;
        auto getView() const -> VkImageView;

    private:
        Resource<VkImage> image;
        Resource<VkDeviceMemory> memory;
        Resource<VkImageView> view;
        Resource<VkSampler> sampler;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

        Image(Resource<VkImage> image, Resource<VkDeviceMemory> memory, Resource<VkImageView> view,
            Resource<VkSampler> sampler, VkImageLayout layout);
    };

    inline auto Image::getLayout() const -> VkImageLayout
    {
        return layout;
    }

    inline auto Image::getSampler() const -> VkSampler
    {
        return sampler;
    }

    inline auto Image::getView() const -> VkImageView
    {
        return view;
    }
}