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
    std::vector<float> data;
    std::vector<uint32_t> indices;

    static auto loadObj(const std::string &path) -> MeshData;
};