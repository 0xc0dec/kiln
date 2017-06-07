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
        explicit DescriptorSetUpdater(VkDevice device);

        auto forUniformBuffer(uint32_t binding, VkDescriptorSet set, VkBuffer buffer,VkDeviceSize offset, VkDeviceSize range) -> DescriptorSetUpdater&;
        auto forTexture(uint32_t binding, VkDescriptorSet set, VkImageView view, VkSampler sampler, VkImageLayout layout) -> DescriptorSetUpdater&;

        void updateSets();

    private:
        struct Item
        {
            VkDescriptorBufferInfo buffer;
            VkDescriptorImageInfo image;
            uint32_t binding;
            VkDescriptorSet targetSet;
        };

        VkDevice device = nullptr;
        std::vector<Item> items;
    };
}