/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "MeshData.h"
#include "FileSystem.h"
#include "Common.h"
#include "StringUtils.h"
#include <glm/gtx/hash.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <utility>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;

    bool operator==(const Vertex &other) const
    {
        return position == other.position && normal == other.normal && texCoord == other.texCoord;
    }
};

namespace std
{
    template<>
    struct hash<Vertex>
    {
        auto operator()(const Vertex &v) const -> size_t
        {
            return (hash<glm::vec3>()(v.position) ^ (hash<glm::vec3>()(v.normal) << 1)) >> 1 ^ (hash<glm::vec2>()(v.texCoord) << 1);
        }
    };
}

static void fromTinyObj(const std::vector<tinyobj::shape_t> &shapes, const tinyobj::attrib_t &attrib,
    std::vector<float> &vertexData, std::vector<uint32_t> &indexData, VertexFormat &format)
{
    std::unordered_map<Vertex, uint32_t> uniqueVertices;

    for (const auto &shape: shapes)
    {
        for (const auto &index: shape.mesh.indices)
        {
            Vertex v;

            if (index.vertex_index >= 0)
            {
                v.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };
            }

            if (index.normal_index >= 0)
            {
                v.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            }

            if (index.texcoord_index >= 0)
            {
                v.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1 - attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }

            if (uniqueVertices.count(v) == 0)
            {
                uniqueVertices[v] = static_cast<uint32_t>(vertexData.size() / 8);
                vertexData.push_back(v.position.x);
                vertexData.push_back(v.position.y);
                vertexData.push_back(v.position.z);
                vertexData.push_back(v.normal.x);
                vertexData.push_back(v.normal.y);
                vertexData.push_back(v.normal.z);
                vertexData.push_back(v.texCoord.x);
                vertexData.push_back(v.texCoord.y);
            }

            indexData.push_back(uniqueVertices[v]);
        }
    }

    format = VertexFormat({3, 3, 2});
}

static bool isLoadable(const std::string &path)
{
    return strutils::endsWith(path, ".obj");
}

auto MeshData::load(const std::string &path) -> MeshData
{
    KL_PANIC_IF(!isLoadable(path));

    auto file = fs::getStream(path);

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    KL_PANIC_IF(!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &file));

    MeshData data;
    fromTinyObj(shapes, attrib, data.vertexData, data.indexData, data.format);

    return data;
}

VertexFormat::VertexFormat(std::vector<uint32_t> attributes):
    attributes(std::move(attributes))
{
}

auto VertexFormat::getSize() const -> uint32_t
{
    uint32_t size = 0;
    for (const auto component : attributes)
        size += component * sizeof(float);
    return size;
}

auto VertexFormat::getAttributeSize(uint32_t attrib) const -> uint32_t
{
    return attributes[attrib] * sizeof(float);
}

auto VertexFormat::getAttributeOffset(uint32_t attrib) const -> uint32_t
{
    uint32_t offset = 0;
    for (auto i = 0; i < attrib; i++)
        offset += attributes[i] * sizeof(float);
    return offset;
}
