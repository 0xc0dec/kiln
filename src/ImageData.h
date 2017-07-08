/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Common.h"
#include <string>

class ImageData
{
public:
    enum class Format
    {
        UNKNOWN = 0,
        R8G8B8A8_UNORM
    };

    ImageData(ImageData &&other) = default;
    ImageData(const ImageData &other) = delete;
    virtual ~ImageData() {}

    auto operator=(const ImageData &other) -> ImageData& = delete;
    auto operator=(ImageData &&other) -> ImageData& = default;

    static auto load2D(const std::string &path) -> uptr<ImageData>;
    static auto loadCube(const std::string &path) -> uptr<ImageData>;

    virtual auto getMipLevelCount() -> uint32_t const = 0;
    virtual auto getFaceCount() -> uint32_t const = 0;

    virtual auto getSize() -> uint32_t const = 0;
    virtual auto getSize(uint32_t mipLevel) -> uint32_t const = 0;
    virtual auto getSize(uint32_t face, uint32_t mipLevel) -> uint32_t const = 0;

    virtual auto getWidth(uint32_t mipLevel) -> uint32_t const = 0;
    virtual auto getWidth(uint32_t face, uint32_t mipLevel) -> uint32_t const = 0;

    virtual auto getHeight(uint32_t mipLevel) -> uint32_t const = 0;
    virtual auto getHeight(uint32_t face, uint32_t mipLevel) -> uint32_t const = 0;

    virtual auto getData() -> void* = 0;

    virtual auto getFormat() -> Format = 0;

protected:
    ImageData() {}
};