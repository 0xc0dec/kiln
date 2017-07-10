/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vulkan.h"

class ImageData;

namespace vk
{
    class Device;

    class Image
    {
    public:
        static auto create2D(const Device &device, const ImageData &data) -> Image;
        static auto createCube(const Device &device, const ImageData &data) -> Image;

        Image() {}
        Image(const Image &other) = delete;
        Image(Image &&other) = default;

        auto operator=(const Image &other) -> Image& = delete;
        auto operator=(Image &&other) -> Image& = default;

        auto getLayout() const -> VkImageLayout { return layout; }
        auto getSampler() const -> VkSampler { return sampler; }
        auto getView() const -> VkImageView { return view; }

    private:
        Resource<VkImage> image;
        Resource<VkDeviceMemory> memory;
        Resource<VkImageView> view;
        Resource<VkSampler> sampler;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

        Image(Resource<VkImage> image, Resource<VkDeviceMemory> memory, Resource<VkImageView> view,
            Resource<VkSampler> sampler, VkImageLayout layout);
    };
}