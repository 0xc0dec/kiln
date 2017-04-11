#pragma once

#include <GL/glew.h>
#include <cstdint>

namespace gl
{
    auto compileShader(GLuint type, const void *src, uint32_t length) -> GLint;
    auto linkShaderProgram(GLuint vs, GLuint fs) -> GLint;

    auto createVertexBuffer(const void *data, uint32_t vertexCount, uint32_t itemsPerVertexCount) -> GLuint;
    auto createIndexBuffer(const void *data, uint32_t elementCount) -> GLuint;
}