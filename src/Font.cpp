/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Font.h"
#include "Vulkan/VulkanImage.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

class TrueTypeFont: public Font
{
public:
    TrueTypeFont(const vk::Device &device, uint8_t *fontData, uint32_t size, uint32_t atlasWidth, uint32_t atlasHeight,
        uint32_t firstChar, uint32_t charCount, uint32_t oversampleX, uint32_t oversampleY):
        Font(device, fontData, size, atlasWidth, atlasHeight, firstChar, charCount, oversampleX, oversampleY),
        firstChar(firstChar)
    {
        charInfo = std::make_unique<stbtt_packedchar[]>(charCount);

        auto pixels = std::make_unique<uint8_t[]>(atlasWidth * atlasHeight);

        stbtt_pack_context context;
        auto ret = stbtt_PackBegin(&context, pixels.get(), atlasWidth, atlasHeight, 0, 1, nullptr);
        KL_PANIC_IF(!ret);

        stbtt_PackSetOversampling(&context, oversampleX, oversampleY);
        stbtt_PackFontRange(&context, fontData, 0, static_cast<float>(size), firstChar, charCount, charInfo.get());
        stbtt_PackEnd(&context);

        atlas = vk::Image(device, atlasWidth, atlasHeight, 1, 1, VK_FORMAT_R8_UNORM,
            0,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_VIEW_TYPE_2D,
            VK_IMAGE_ASPECT_COLOR_BIT);
    }

    auto getGlyphInfo(uint32_t character, float offsetX, float offsetY) -> GlyphInfo override
    {
        stbtt_aligned_quad quad;
        auto atlasSize = atlas.getSize();

        stbtt_GetPackedQuad(charInfo.get(), static_cast<uint32_t>(atlasSize.x), static_cast<uint32_t>(atlasSize.y),
        character - firstChar, &offsetX, &offsetY, &quad, 1);
        auto xmin = quad.x0;
        auto xmax = quad.x1;
        auto ymin = -quad.y1;
        auto ymax = -quad.y0;

        GlyphInfo result;
        result.offsetX = offsetX;
        result.offsetY = offsetY;
        result.positions[0] = {xmin, ymin, 0};
        result.positions[1] = {xmin, ymax, 0};
        result.positions[2] = {xmax, ymax, 0};
        result.positions[3] = {xmax, ymin, 0};
        result.uvs[0] = {quad.s0, quad.t1};
        result.uvs[1] = {quad.s0, quad.t0};
        result.uvs[2] = {quad.s1, quad.t0};
        result.uvs[3] = {quad.s1, quad.t1};

        return result; // TODO move
    }

private:
    uint32_t firstChar;
    uptr<stbtt_packedchar[]> charInfo;
    vk::Image atlas;
};

Font::Font(const vk::Device &device, uint8_t *fontData, uint32_t size, uint32_t atlasWidth, uint32_t atlasHeight,
    uint32_t firstChar, uint32_t charCount, uint32_t oversampleX, uint32_t oversampleY)
{
    impl = std::make_unique<TrueTypeFont>(device, fontData, size, atlasWidth, atlasHeight, firstChar,
        charCount, oversampleX, oversampleY);
}

auto Font::getGlyphInfo(uint32_t character, float offsetX, float offsetY) -> GlyphInfo
{
    return impl->getGlyphInfo(character, offsetX, offsetY);
}
