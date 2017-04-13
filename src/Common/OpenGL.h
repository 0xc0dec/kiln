/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include <GL/glew.h>
#include <cstdint>

namespace gl
{
    auto createShaderProgram(const void *vsSrc, uint32_t vsSrcLen, const void *fsSrc, uint32_t fsSrcLen) -> GLuint;

    auto createVertexBuffer(const void *data, uint32_t vertexCount, uint32_t itemsPerVertexCount) -> GLuint;
    auto createIndexBuffer(const void *data, uint32_t elementCount) -> GLuint;
}