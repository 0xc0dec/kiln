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

        auto getVertexShader() const -> VkShaderModule { return vertexShader; }
        auto getFragmentShader() const -> VkShaderModule { return fragmentShader; }

    private:
        Resource<VkShaderModule> vertexShader;
        Resource<VkShaderModule> fragmentShader;
    };
}
