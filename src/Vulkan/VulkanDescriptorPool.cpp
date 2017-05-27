/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "VulkanDescriptorPool.h"

vk::DescriptorPool::DescriptorPool(VkDevice device, Resource<VkDescriptorPool> pool):
    device(device),
    pool(std::move(pool))
{
}

// TODO use builder
vk::DescriptorPool::DescriptorPool(VkDevice device, const std::vector<VkDescriptorType> &descriptorTypes,
    const std::vector<uint32_t> &descriptorCounts, uint32_t maxSetCount):
    device(device)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (size_t i = 0; i < descriptorTypes.size(); ++i)
    {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = descriptorTypes[i];
        poolSize.descriptorCount = descriptorCounts[i];
        poolSizes.push_back(poolSize);
    }

    VkDescriptorPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxSetCount;

    this->pool = Resource<VkDescriptorPool>{device, vkDestroyDescriptorPool};
    KL_VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, pool.cleanRef()));
}

vk::DescriptorPool::DescriptorPool(DescriptorPool &&other) noexcept
{
    swap(other);
}

auto vk::DescriptorPool::operator=(DescriptorPool other) noexcept -> DescriptorPool&
{
    swap(other);
    return *this;
}

auto vk::DescriptorPool::allocateSet(VkDescriptorSetLayout layout) const -> VkDescriptorSet
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet set;
    KL_VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &set));

    return set;
}

void vk::DescriptorPool::swap(DescriptorPool &other) noexcept
{
    std::swap(device, other.device);
    std::swap(pool, other.pool);
}

vk::DescriptorPoolBuilder::DescriptorPoolBuilder(VkDevice device):
    device(device)
{
}

auto vk::DescriptorPoolBuilder::forDescriptors(VkDescriptorType descriptorType, uint32_t descriptorCount) -> DescriptorPoolBuilder&
{
    VkDescriptorPoolSize poolSize{};
    poolSize.type = descriptorType;
    poolSize.descriptorCount = descriptorCount;
    sizes.push_back(poolSize);
    return *this;
}

auto vk::DescriptorPoolBuilder::build(uint32_t maxSetCount) -> DescriptorPool
{
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = sizes.size();
    poolInfo.pPoolSizes = sizes.data();
    poolInfo.maxSets = maxSetCount;

    Resource<VkDescriptorPool> pool{device, vkDestroyDescriptorPool};
    KL_VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, pool.cleanRef()));

    return DescriptorPool(device, std::move(pool));
}
