/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Common.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanImage.h"
#include <glm/glm.hpp>

class Font
{
public:
    struct GlyphInfo
    {
        glm::vec3 positions[4];
        glm::vec2 uvs[4];
        float offsetX, offsetY;
    };

    static auto createTrueType(const vk::Device &device, const std::vector<uint8_t> &data, float size,
        uint32_t atlasWidth, uint32_t atlasHeight, uint32_t firstChar, uint32_t charCount,
        uint32_t oversampleX, uint32_t oversampleY) -> Font;

    Font() {}
    Font(const Font &other) = delete;
    Font(Font &&other) = default;
    virtual ~Font() = default;

    auto operator=(const Font &other) -> Font& = delete;
    auto operator=(Font &&other) -> Font& = default;

    virtual auto getGlyphInfo(uint32_t character, float offsetX, float offsetY) -> GlyphInfo
    {
        return impl->getGlyphInfo(character, offsetX, offsetY);
    }

    auto getAtlas() const -> const vk::Image& { return impl->atlas; }

protected:
    vk::Image atlas;

private:
    uptr<Font> impl;
};