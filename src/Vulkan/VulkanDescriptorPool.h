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
        DescriptorPool(VkDevice device, Resource<VkDescriptorPool> pool);
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

    class DescriptorPoolBuilder
    {
    public:
        explicit DescriptorPoolBuilder(VkDevice device);

        auto forDescriptors(VkDescriptorType descriptorType, uint32_t descriptorCount) -> DescriptorPoolBuilder&;
        auto build(uint32_t maxSetCount) -> DescriptorPool;

    private:
        VkDevice device = nullptr;
        std::vector<VkDescriptorPoolSize> sizes;
    };
}