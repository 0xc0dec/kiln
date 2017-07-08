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
        DescriptorPool(DescriptorPool &&other) = default;
        ~DescriptorPool() {}

        auto operator=(const DescriptorPool &other) -> DescriptorPool& = delete;
        auto operator=(DescriptorPool &&other) -> DescriptorPool& = default;

        auto allocateSet(VkDescriptorSetLayout layout) const -> VkDescriptorSet;

    private:
        VkDevice device = nullptr;
        Resource<VkDescriptorPool> pool;
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