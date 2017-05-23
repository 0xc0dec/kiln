/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Image.h"
#include <png.h>
#include <cassert>

struct ReadContext
{
    uint8_t *data;
    uint32_t offset;
};

static void callback(png_structp png, png_bytep data, png_size_t length)
{
    auto context = reinterpret_cast<ReadContext*>(png_get_io_ptr(png));
    memcpy(data, context->data + context->offset, length);
    context->offset += length;
}

auto img::loadPNG(const std::vector<uint8_t> &bytes) -> Image
{
    assert(bytes.size() >= 8 && png_sig_cmp(&bytes[0], 0, 8) == 0); // TODO

    auto png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    auto info = png_create_info_struct(png);
    if (setjmp(png_jmpbuf(png)))
    {
        png_destroy_info_struct(png, &info);
        png_destroy_read_struct(&png, &info, nullptr);
        assert(false /* Failed to read PNG file*/); // TODO
    }

    ReadContext context{const_cast<uint8_t*>(bytes.data()), 8};
    png_set_read_fn(png, reinterpret_cast<png_voidp>(&context), callback);
    png_set_sig_bytes(png, 8);
    png_read_png(png, info, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, nullptr);

    auto width = png_get_image_width(png, info);
    auto height = png_get_image_height(png, info);
    auto colorType = png_get_color_type(png, info);

    auto format = ImageFormat::RGB;
    switch (colorType)
    {
        case PNG_COLOR_TYPE_RGB:
            format = ImageFormat::RGB;
            break;
        case PNG_COLOR_TYPE_RGBA:
            format = ImageFormat::RGBA;
            break;
        default:
            png_destroy_info_struct(png, &info);
            png_destroy_read_struct(&png, &info, nullptr);
            assert(false /* Unsupported PNG color type */); // TODO
    }

    auto stride = png_get_rowbytes(png, info);

    Image image;
    image.width = static_cast<uint32_t>(width);
    image.height = static_cast<uint32_t>(height);
    image.format = format;
    image.data.resize(stride * height);

    auto rows = png_get_rows(png, info);
    for (uint32_t i = 0; i < height; ++i)
        memcpy(image.data.data() + stride * (height - i - 1), rows[i], stride);

    png_destroy_info_struct(png, &info);
    png_destroy_read_struct(&png, &info, nullptr);

    return image;
}
