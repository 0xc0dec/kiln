/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "ImageData.h"
#include "FileSystem.h"
#include <gli/gli.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

class GliImageData : public ImageData
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

    static auto load2D(const std::string &path) -> uptr<GliImageData>
    {
        auto bytes = fs::readBytes(path);
        gli::texture2d data(gli::load(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
        return std::unique_ptr<GliImageData>(new GliImageData(std::move(data)));
    }

    static auto loadCube(const std::string &path) -> uptr<GliImageData>
    {
        auto bytes = fs::readBytes(path);
        gli::texture_cube data(gli::load(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
        return std::unique_ptr<GliImageData>(new GliImageData(std::move(data)));
    }

    auto getMipLevelCount() -> uint32_t const override
    {
        return getGenericHandle().levels();
    }

    auto getFaceCount() -> uint32_t const override
    {
        return getGenericHandle().faces();
    }

    auto getSize() -> uint32_t const override
    {
        return getGenericHandle().size();
    }

    auto getSize(uint32_t mipLevel) -> uint32_t const override
    {
        return getGenericHandle().size(mipLevel);
    }

    auto getSize(uint32_t face, uint32_t mipLevel) -> uint32_t const override
    {
        return texCube[face][mipLevel].size();
    }

    auto getWidth(uint32_t mipLevel) -> uint32_t const override
    {
        return getGenericHandle().extent(mipLevel).x;
    }

    auto getWidth(uint32_t face, uint32_t mipLevel) -> uint32_t const override
    {
        return texCube[face][mipLevel].extent().x;
    }

    auto getHeight(uint32_t mipLevel) -> uint32_t const override
    {
        return getGenericHandle().extent(mipLevel).y;
    }

    auto getHeight(uint32_t face, uint32_t mipLevel) -> uint32_t const override
    {
        return texCube[face][mipLevel].extent().y;
    }

    auto getData() -> void* override
    {
        return getGenericHandle().data();
    }

    auto getFormat() -> Format override
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

    gli::texture2d tex2d;
    gli::texture_cube texCube;

    static bool isLoadable(const std::string &path)
    {
        for (const auto &ext : supportedFormats)
        {
            if (std::equal(ext.rbegin(), ext.rend(), path.rbegin()))
                return true;
        }
        return false;
    }

    explicit GliImageData(gli::texture2d &&t): tex2d(std::move(t))
    {
    }

    explicit GliImageData(gli::texture_cube &&t): texCube(std::move(t))
    {
    }

    auto getGenericHandle() -> gli::texture&
    {
        if (!tex2d.empty())
            return tex2d;
        return texCube;
    }
};

decltype(GliImageData::supportedFormats) GliImageData::supportedFormats = {".dds", ".ktx"};

class StbiImageData: public ImageData
{
public:
    static bool isLoadable2D(const std::string &path)
    {
        for (const auto &ext : supportedFormats)
        {
            if (std::equal(ext.rbegin(), ext.rend(), path.rbegin()))
                return true;
        }
        return false;
    }

    static bool isLoadableCube(const std::string &path)
    {
        return false;
    }

    static auto load2D(const std::string &path) -> uptr<StbiImageData>
    {
        auto bytes = fs::readBytes(path);
        int width, height, channels;
        auto data = stbi_load_from_memory(bytes.data(), bytes.size(), &width, &height, &channels, STBI_rgb_alpha);
        return std::unique_ptr<StbiImageData>(new StbiImageData(width, height, channels, data));
    }

    static auto loadCube(const std::string &path) -> uptr<GliImageData>
    {
        KL_PANIC("Not implemented");
        return nullptr;
    }

    ~StbiImageData()
    {
        stbi_image_free(data);
    }

    auto getMipLevelCount() -> uint32_t const override
    {
        return 1;
    }

    auto getFaceCount() -> uint32_t const override
    {
        return 1;
    }

    auto getSize() -> uint32_t const override
    {
        return width * height * channels;
    }

    auto getSize(uint32_t mipLevel) -> uint32_t const override
    {
        return getSize();
    }

    auto getSize(uint32_t face, uint32_t mipLevel) -> uint32_t const override
    {
        return getSize();
    }

    auto getWidth(uint32_t mipLevel) -> uint32_t const override
    {
        return width;
    }

    auto getWidth(uint32_t face, uint32_t mipLevel) -> uint32_t const override
    {
        return width;
    }

    auto getHeight(uint32_t mipLevel) -> uint32_t const override
    {
        return height;
    }

    auto getHeight(uint32_t face, uint32_t mipLevel) -> uint32_t const override
    {
        return height;
    }

    auto getData() -> void* override
    {
        return data;
    }

    auto getFormat() -> Format override
    {
        return Format::R8G8B8A8_UNORM;
    }

private:
    static const std::vector<std::string> supportedFormats;

    uint32_t channels = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    stbi_uc *data = nullptr;

    StbiImageData(uint32_t width, uint32_t height, uint32_t channels, stbi_uc *data):
        channels(channels), width(width), height(height), data(data)
    {
    }
};

decltype(StbiImageData::supportedFormats) StbiImageData::supportedFormats = {".bmp", ".jpg", ".jpeg", ".png"};

auto ImageData::load2D(const std::string &path) -> uptr<ImageData>
{
    if (GliImageData::isLoadable2D(path))
        return GliImageData::load2D(path);
    if (StbiImageData::isLoadable2D(path))
        return StbiImageData::load2D(path);
    KL_PANIC("Unsupported texture format");
    return nullptr;
}

auto ImageData::loadCube(const std::string &path) -> uptr<ImageData>
{
    if (GliImageData::isLoadableCube(path))
        return GliImageData::loadCube(path);
    if (StbiImageData::isLoadableCube(path))
        return StbiImageData::loadCube(path);
    KL_PANIC("Unsupported texture format");
    return nullptr;
}
