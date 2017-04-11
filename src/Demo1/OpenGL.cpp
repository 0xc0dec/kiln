#include "OpenGL.h"
#include <unordered_map>
#include <string>
#include <cassert>


auto gl::compileShader(GLuint type, const void *src, uint32_t length) -> GLint
{
    static std::unordered_map<GLuint, std::string> typeNames =
    {
        {GL_VERTEX_SHADER, "vertex"},
        {GL_FRAGMENT_SHADER, "fragment"}
    };

    auto shader = glCreateShader(type);

    GLint len = length;
    glShaderSource(shader, 1, reinterpret_cast<const GLchar* const*>(&src), &len);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<GLchar> log(logLength);
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        glDeleteShader(shader);
        assert(false /* Failed to compile shader */); // TODO
    }

    return shader;
}


auto gl::linkShaderProgram(GLuint vs, GLuint fs) -> GLint
{
    auto program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint logLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<GLchar> log(logLength);
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        glDeleteProgram(program);
        assert(false /* Failed to link program */); // TODO
    }

    return program;
}


auto gl::createVertexBuffer(const void* data, uint32_t vertexCount, uint32_t itemsPerVertexCount) -> GLuint
{
    GLuint handle = 0;
    glGenBuffers(1, &handle);
    assert(handle);

    glBindBuffer(GL_ARRAY_BUFFER, handle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * itemsPerVertexCount * vertexCount, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return handle;
}


auto gl::createIndexBuffer(const void* data, uint32_t elementCount) -> GLuint
{
    GLuint handle = 0;
    glGenBuffers(1, &handle);
    assert(handle);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * elementCount, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return handle;
}
