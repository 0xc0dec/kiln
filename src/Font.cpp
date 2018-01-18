/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Font.h"
#include "ImageData.h"
#include "Vulkan/VulkanImage.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

class TrueTypeFont: public Font
{
public:
    TrueTypeFont(const vk::Device &device, const std::vector<uint8_t> &data, float size, uint32_t atlasWidth, uint32_t atlasHeight,
        uint32_t firstChar, uint32_t charCount, uint32_t oversampleX, uint32_t oversampleY):
        firstChar(firstChar)
    {
        charInfo = std::make_unique<stbtt_packedchar[]>(charCount);

        std::vector<uint8_t> pixels;
        pixels.resize(atlasWidth * atlasHeight);

        stbtt_pack_context context;
	    const auto ret = stbtt_PackBegin(&context, pixels.data(), atlasWidth, atlasHeight, 0, 1, nullptr);
        KL_PANIC_IF(!ret);

        stbtt_PackSetOversampling(&context, oversampleX, oversampleY);
        stbtt_PackFontRange(&context, const_cast<unsigned char *>(data.data()), 0, size, firstChar, charCount, charInfo.get());
        stbtt_PackEnd(&context);

	    const auto imageData = ImageData::createSimple(atlasWidth, atlasHeight, ImageData::Format::R8_UNORM, pixels);        

        atlas = vk::Image(device, atlasWidth, atlasHeight, 1, 1, VK_FORMAT_R8_UNORM,
            0,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_VIEW_TYPE_2D,
            VK_IMAGE_ASPECT_COLOR_BIT);
        atlas.uploadData(device, imageData);
    }

    auto getGlyphInfo(uint32_t character, float offsetX, float offsetY) -> GlyphInfo override
    {
        stbtt_aligned_quad quad;
        const auto atlasSize = atlas.getSize();

        stbtt_GetPackedQuad(charInfo.get(), static_cast<uint32_t>(atlasSize.x), static_cast<uint32_t>(atlasSize.y),
            character - firstChar, &offsetX, &offsetY, &quad, 1);
        const auto xmin = quad.x0;
        const auto xmax = quad.x1;
        const auto ymin = -quad.y1;
        const auto ymax = -quad.y0;

        GlyphInfo result;
        result.offsetX = offsetX;
        result.offsetY = offsetY;
        result.positions[0] = {xmin, -ymin, 0};
        result.positions[1] = {xmin, -ymax, 0};
        result.positions[2] = {xmax, -ymax, 0};
        result.positions[3] = {xmax, -ymin, 0};
        result.uvs[0] = {quad.s0, quad.t1};
        result.uvs[1] = {quad.s0, quad.t0};
        result.uvs[2] = {quad.s1, quad.t0};
        result.uvs[3] = {quad.s1, quad.t1};

        return result; // TODO move
    }

private:
    uint32_t firstChar;
    uptr<stbtt_packedchar[]> charInfo;
};

auto Font::createTrueType(const vk::Device &device, const std::vector<uint8_t> &data, float size,
    uint32_t atlasWidth, uint32_t atlasHeight, uint32_t firstChar, uint32_t charCount,
    uint32_t oversampleX, uint32_t oversampleY) -> Font
{
    Font f;
    f.impl = std::make_unique<TrueTypeFont>(device, data, size, atlasWidth, atlasHeight, firstChar, charCount, oversampleX, oversampleY);
    return f;
}
