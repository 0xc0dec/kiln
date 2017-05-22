/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vulkan.h"
#include <vector>

namespace vk
{
    class DescriptorSetLayoutBuilder
    {
    public:
        explicit DescriptorSetLayoutBuilder(VkDevice device);

        auto withBinding(uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount,
            VkShaderStageFlagBits stageFlags) -> DescriptorSetLayoutBuilder&;

        auto build() -> Resource<VkDescriptorSetLayout>;

    private:
        VkDevice device = nullptr;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
    };
}