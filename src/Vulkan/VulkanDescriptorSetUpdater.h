/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vulkan.h"
#include <vector>

namespace vk
{
    class DescriptorSetUpdater
    {
    public:
        auto forUniformBuffer(uint32_t binding, VkDescriptorSet set, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) -> DescriptorSetUpdater&;
        auto forTexture(uint32_t binding, VkDescriptorSet set, VkImageView view, VkSampler sampler, VkImageLayout layout) -> DescriptorSetUpdater&;

        void updateSets(VkDevice device);

    private:
        struct Buffer
        {
            VkDescriptorBufferInfo info;
            uint32_t binding;
            VkDescriptorSet targetSet;
        };

        struct Texture
        {
            VkDescriptorImageInfo info;
            uint32_t binding;
            VkDescriptorSet targetSet;
        };

        std::vector<Buffer> buffers;
        std::vector<Texture> textures;
    };
}