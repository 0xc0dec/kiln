/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "ImageData.h"
#include "FileSystem.h"
#include "StringUtils.h"
#include <gli/gli.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

class GliData : public ImageData
{
public:
    static bool isLoadable2D(const std::string &path)
    {
        return isLoadable(path);
    }

    static bool isLoadableCube(const std::string &path)
    {
        return isLoadable(path);
    }

    static auto load2D(const std::string &path) -> uptr<GliData>
    {
        auto bytes = fs::readBytes(path);
        gli::texture2d data(gli::load(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
        return std::unique_ptr<GliData>(new GliData(std::move(data)));
    }

    static auto loadCube(const std::string &path) -> uptr<GliData>
    {
        auto bytes = fs::readBytes(path);
        gli::texture_cube data(gli::load(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
        return std::unique_ptr<GliData>(new GliData(std::move(data)));
    }

    auto getMipLevelCount() const -> uint32_t override
    {
        return getGenericHandle().levels();
    }

    auto getFaceCount() const -> uint32_t override
    {
        return getGenericHandle().faces();
    }

    auto getSize() const -> uint32_t override
    {
        return getGenericHandle().size();
    }

    auto getSize(uint32_t mipLevel) const -> uint32_t override
    {
        return getGenericHandle().size(mipLevel);
    }

    auto getSize(uint32_t face, uint32_t mipLevel) const -> uint32_t override
    {
        return texCube[face][mipLevel].size();
    }

    auto getWidth(uint32_t mipLevel) const -> uint32_t override
    {
        return getGenericHandle().extent(mipLevel).x;
    }

    auto getWidth(uint32_t face, uint32_t mipLevel) const -> uint32_t override
    {
        return texCube[face][mipLevel].extent().x;
    }

    auto getHeight(uint32_t mipLevel) const -> uint32_t override
    {
        return getGenericHandle().extent(mipLevel).y;
    }

    auto getHeight(uint32_t face, uint32_t mipLevel) const -> uint32_t override
    {
        return texCube[face][mipLevel].extent().y;
    }

    auto getData() const -> const void* override
    {
        return getGenericHandle().data();
    }

    auto getFormat() const -> Format override
    {
        switch (getGenericHandle().format())
        {
            case gli::FORMAT_RGBA8_UNORM_PACK8:
                return Format::R8G8B8A8_UNORM;
            default:
                KL_PANIC("Unsupported texture data format");
                return Format::UNKNOWN;
        }
    }

private:
    static const std::vector<std::string> supportedFormats;

    mutable gli::texture2d tex2d;
    mutable gli::texture_cube texCube;

    static bool isLoadable(const std::string &path)
    {
        for (const auto &ext : supportedFormats)
        {
            if (strutils::endsWith(path, ext))
                return true;
        }
        return false;
    }

    explicit GliData(gli::texture2d &&t): tex2d(std::move(t))
    {
    }

    explicit GliData(gli::texture_cube &&t): texCube(std::move(t))
    {
    }

    auto getGenericHandle() const -> gli::texture&
    {
        if (!tex2d.empty())
            return tex2d;
        return texCube;
    }
};

decltype(GliData::supportedFormats) GliData::supportedFormats = {".dds", ".ktx"};

class StbiData: public ImageData
{
public:
    static bool isLoadable2D(const std::string &path)
    {
        for (const auto &ext : supportedFormats)
        {
            if (strutils::endsWith(path, ext))
                return true;
        }
        return false;
    }

    static bool isLoadableCube(const std::string &path)
    {
        return false;
    }

    static auto load2D(const std::string &path) -> uptr<StbiData>
    {
        auto bytes = fs::readBytes(path);
        int width, height, channels;
        auto data = stbi_load_from_memory(bytes.data(), bytes.size(), &width, &height, &channels, STBI_rgb_alpha);
        return std::unique_ptr<StbiData>(new StbiData(width, height, channels, data));
    }

    static auto loadCube(const std::string &path) -> uptr<GliData>
    {
        KL_PANIC("Not implemented");
        return nullptr;
    }

    ~StbiData()
    {
        if (data)
            stbi_image_free(data);
    }

    auto getMipLevelCount() const -> uint32_t override
    {
        return 1;
    }

    auto getFaceCount() const -> uint32_t override
    {
        return 1;
    }

    auto getSize() const -> uint32_t override
    {
        return width * height * channels;
    }

    auto getSize(uint32_t mipLevel) const -> uint32_t override
    {
        return getSize();
    }

    auto getSize(uint32_t face, uint32_t mipLevel) const -> uint32_t override
    {
        return getSize();
    }

    auto getWidth(uint32_t mipLevel) const -> uint32_t override
    {
        return width;
    }

    auto getWidth(uint32_t face, uint32_t mipLevel) const -> uint32_t override
    {
        return width;
    }

    auto getHeight(uint32_t mipLevel) const -> uint32_t override
    {
        return height;
    }

    auto getHeight(uint32_t face, uint32_t mipLevel) const -> uint32_t override
    {
        return height;
    }

    auto getData() const -> const void* override
    {
        return data;
    }

    auto getFormat() const -> Format override
    {
        return Format::R8G8B8A8_UNORM;
    }

private:
    static const std::vector<std::string> supportedFormats;

    uint32_t channels = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    stbi_uc *data = nullptr;

    StbiData(uint32_t width, uint32_t height, uint32_t channels, stbi_uc *data):
        channels(channels), width(width), height(height), data(data)
    {
    }
};

class SimpleData: public ImageData
{
public:
    SimpleData(uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layers, uint32_t faces, Format format, const std::vector<uint8_t> &data):
        width(width),
        height(height),
        mipLevels(mipLevels),
        layers(layers),
        faces(faces),
        format(format),
        data(data)
    {
    }

    auto getMipLevelCount() const -> uint32_t override { return mipLevels; }
    auto getFaceCount() const -> uint32_t override { return faces; }
    auto getSize() const -> uint32_t override { return data.size(); }
    auto getSize(uint32_t mipLevel) const -> uint32_t override { return data.size(); }
    auto getSize(uint32_t face, uint32_t mipLevel) const -> uint32_t override { return data.size(); }
    auto getWidth(uint32_t mipLevel) const -> uint32_t override { return width; }
    auto getWidth(uint32_t face, uint32_t mipLevel) const -> uint32_t override { return width; }
    auto getHeight(uint32_t mipLevel) const -> uint32_t override { return height; }
    auto getHeight(uint32_t face, uint32_t mipLevel) const -> uint32_t override { return height; }
    auto getData() const -> const void* override { return data.data(); }
    auto getFormat() const -> Format override { return format; }

private:
    uint32_t width;
    uint32_t height;
    uint32_t mipLevels;
    uint32_t layers;
    uint32_t faces;
    Format format;
    std::vector<uint8_t> data;
};

decltype(StbiData::supportedFormats) StbiData::supportedFormats = {".bmp", ".jpg", ".jpeg", ".png"};

auto ImageData::load2D(const std::string &path) -> ImageData
{
    ImageData data{};
    if (GliData::isLoadable2D(path))
        data.impl = GliData::load2D(path);
    else if (StbiData::isLoadable2D(path))
        data.impl = StbiData::load2D(path);
    else
        KL_PANIC("Unsupported texture format");
    return data;
}

auto ImageData::loadCube(const std::string &path) -> ImageData
{
    ImageData data{};
    if (GliData::isLoadableCube(path))
        data.impl = GliData::loadCube(path);
    else if (StbiData::isLoadableCube(path))
        data.impl = StbiData::loadCube(path);
    else
        KL_PANIC("Unsupported texture format");
    return data;
}

auto ImageData::createSimple(uint32_t width, uint32_t height, Format format, const std::vector<uint8_t> &data) -> ImageData
{
    ImageData result;
    result.impl = std::make_unique<SimpleData>(width, height, 1, 1, 1, format, data);
    return result;
}
