/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vulkan.h"

namespace vk
{
    class DescriptorPool
    {
    public:
        DescriptorPool() {}
        DescriptorPool(VkDevice device, VkDescriptorType type, uint32_t descriptorCount, uint32_t maxSetCount);
        DescriptorPool(const DescriptorPool &other) = delete;
        DescriptorPool(DescriptorPool &&other) noexcept;

        ~DescriptorPool() {}

        auto operator=(DescriptorPool other) noexcept -> DescriptorPool&;

        auto allocateSet(VkDescriptorSetLayout layout) const -> VkDescriptorSet;

    private:
        VkDevice device = nullptr;
        Resource<VkDescriptorPool> pool;

        void swap(DescriptorPool &other) noexcept;
    };
}