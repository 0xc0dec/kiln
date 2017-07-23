/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include <vector>
#include <glm/glm.hpp>

class VertexFormat
{
public:
    VertexFormat() {}
    explicit VertexFormat(const std::vector<uint32_t> &attributes);

    auto getSize() const -> uint32_t;
    auto getAttributeCount() const { return attributes.size(); }
    auto getAttributeSize(uint32_t attrib) const -> uint32_t;
    auto getAttributeOffset(uint32_t attrib) const -> uint32_t;

private:
    std::vector<uint32_t> attributes;
};

class MeshData
{
public:
    MeshData(const MeshData &other) = delete;
    MeshData(MeshData &&other) = default;

    auto operator=(const MeshData &other) -> MeshData& = delete;
    auto operator=(MeshData &&other) -> MeshData& = default;

    static auto load(const std::string &path) -> MeshData;

    auto getFormat() const { return format; }
    auto getVertexData() const -> const std::vector<float>& { return vertexData; }
    auto getIndexData() const -> const std::vector<uint32_t>& { return indexData; }

private:
    VertexFormat format;
    std::vector<float> vertexData;
    std::vector<uint32_t> indexData;

    MeshData() {}
};