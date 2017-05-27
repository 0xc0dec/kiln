/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vulkan.h"
#include <vector>

namespace vk
{
    class DescriptorPool
    {
    public:
        DescriptorPool() {}
        DescriptorPool(VkDevice device, const std::vector<VkDescriptorType> &descriptorTypes,
            const std::vector<uint32_t> &descriptorCounts, uint32_t maxSetCount);
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