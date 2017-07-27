/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Common.h"
#include <string>
#include <vector>

class ImageData
{
public:
    enum class Format
    {
        UNKNOWN = 0,
        R8_UNORM,
        R8G8B8A8_UNORM
    };

    static auto load2D(const std::string &path) -> ImageData;
    static auto loadCube(const std::string &path) -> ImageData;
    static auto createSimple(uint32_t width, uint32_t height, Format format, const std::vector<uint8_t> &data) -> ImageData;

    ImageData(ImageData &&other) = default;
    ImageData(const ImageData &other) = delete;
    virtual ~ImageData() {}

    auto operator=(const ImageData &other) -> ImageData& = delete;
    auto operator=(ImageData &&other) -> ImageData& = default;

    virtual auto getMipLevelCount() const -> uint32_t { return impl->getMipLevelCount(); }
    virtual auto getFaceCount() const -> uint32_t { return impl->getFaceCount(); }

    virtual auto getSize() const -> uint32_t { return impl->getSize(); }
    virtual auto getSize(uint32_t mipLevel) const -> uint32_t { return impl->getSize(mipLevel); }
    virtual auto getSize(uint32_t face, uint32_t mipLevel) const -> uint32_t { return impl->getSize(face, mipLevel); }

    virtual auto getWidth(uint32_t mipLevel) const -> uint32_t { return impl->getWidth(mipLevel); }
    virtual auto getWidth(uint32_t face, uint32_t mipLevel) const -> uint32_t { return impl->getWidth(face, mipLevel); }

    virtual auto getHeight(uint32_t mipLevel) const -> uint32_t { return impl->getHeight(mipLevel); }
    virtual auto getHeight(uint32_t face, uint32_t mipLevel) const -> uint32_t { return impl->getHeight(face, mipLevel); }

    virtual auto getData() const -> const void* { return impl->getData(); }

    virtual auto getFormat() const -> Format { return impl->getFormat(); }

protected:
    ImageData() {}

private:
    uptr<ImageData> impl = nullptr;
};