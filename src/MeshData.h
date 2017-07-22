/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include <vector>
#include <glm/glm.hpp>

class MeshData
{
public:
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

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    static auto loadObj(const std::string &path) -> MeshData;
};