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

    static auto load2D(const std::string &path) -> ImageData;
    static auto loadCube(const std::string &path) -> ImageData;

    ImageData(ImageData &&other) = default;
    ImageData(const ImageData &other) = delete;
    virtual ~ImageData() {}

    auto operator=(const ImageData &other) -> ImageData& = delete;
    auto operator=(ImageData &&other) -> ImageData& = default;

    virtual auto getMipLevelCount() const -> uint32_t;
    virtual auto getFaceCount() const -> uint32_t;

    virtual auto getSize() const -> uint32_t;
    virtual auto getSize(uint32_t mipLevel) const -> uint32_t;
    virtual auto getSize(uint32_t face, uint32_t mipLevel) const -> uint32_t;

    virtual auto getWidth(uint32_t mipLevel) const -> uint32_t;
    virtual auto getWidth(uint32_t face, uint32_t mipLevel) const -> uint32_t;

    virtual auto getHeight(uint32_t mipLevel) const -> uint32_t;
    virtual auto getHeight(uint32_t face, uint32_t mipLevel) const -> uint32_t;

    virtual auto getData() const -> void*;

    virtual auto getFormat() const -> Format;

protected:
    ImageData() {}

private:
    uptr<ImageData> impl = nullptr;
};