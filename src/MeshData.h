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
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<std::vector<uint32_t>> indices;

    static auto loadObj(const std::string &path) -> MeshData;
};