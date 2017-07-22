/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "MeshData.h"
#include "FileSystem.h"
#include "Common.h"
#include <glm/gtx/hash.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>

namespace std
{
    template<>
    struct hash<MeshData::Vertex>
    {
        auto operator()(const MeshData::Vertex &v) const -> size_t
        {
            return (hash<glm::vec3>()(v.position) ^ (hash<glm::vec3>()(v.normal) << 1)) >> 1 ^ (hash<glm::vec2>()(v.texCoord) << 1);
        }
    };
}

auto MeshData::loadObj(const std::string &path) -> MeshData
{
    std::ifstream file{path};
    KL_PANIC_IF(!file.is_open());

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    KL_PANIC_IF(!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &file));

    file.close();

    std::unordered_map<Vertex, uint32_t> uniqueVertices;
    MeshData data;

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
                uniqueVertices[v] = static_cast<uint32_t>(data.vertices.size());
                data.vertices.push_back(v);
            }

            data.indices.push_back(uniqueVertices[v]);
        }
    }

    return std::move(data);
}
