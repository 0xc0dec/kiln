/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Common.h"
#include "Vulkan/VulkanDevice.h"
#include <glm/glm.hpp>

struct GlyphInfo
{
    glm::vec3 positions[4];
    glm::vec2 uvs[2];
    float offsetX, offsetY;
};

class Font
{
public:
    Font(const vk::Device &device, uint8_t *fontData, uint32_t size, uint32_t atlasWidth, uint32_t atlasHeight,
        uint32_t firstChar, uint32_t charCount, uint32_t oversampleX, uint32_t oversampleY);
    Font(const Font &other) = delete;
    Font(Font &&other) = default;
    virtual ~Font() = default;

    auto operator=(const Font &other) -> Font& = delete;
    auto operator=(Font &&other) -> Font& = default;

    virtual auto getGlyphInfo(uint32_t character, float offsetX, float offsetY) -> GlyphInfo = 0;

private:
    uptr<Font> impl;
};