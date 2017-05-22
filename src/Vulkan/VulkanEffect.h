/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vulkan.h"

namespace vk
{
    class Effect final
    {
    public:
        Effect(const std::string &vsSrc, const std::string &fsSrc);
        ~Effect();

        auto getVertexShader() const -> VkShaderModule;
        auto getFragmentShader() const -> VkShaderModule;

    private:
        Resource<VkShaderModule> vertexShader;
        Resource<VkShaderModule> fragmentShader;
    };

    inline auto Effect::getVertexShader() const -> VkShaderModule
    {
        return vertexShader;
    }

    inline auto Effect::getFragmentShader() const -> VkShaderModule
    {
        return fragmentShader;
    }
}
