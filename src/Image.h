/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include <cstdint>
#include <vector>

namespace img
{
    enum class ImageFormat
    {
        RGB,
        RGBA
    };

    struct Image
    {
        uint32_t width;
        uint32_t height;
        ImageFormat format;
        std::vector<uint8_t> data;
    };

    auto loadPNG(const std::vector<uint8_t> &bytes) -> Image;
}