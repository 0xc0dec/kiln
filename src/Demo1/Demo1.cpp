/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "OpenGLWindow.h"
#include "Transform.h"
#include "Degree.h"
#include <SDL.h>
#include <GL/glew.h>
#include <vector>
#include <cassert>
#include <unordered_map>


static auto compileShader(GLuint type, const void *src, uint32_t length) -> GLint
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


static auto linkProgram(GLuint vs, GLuint fs) -> GLint
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


bool shouldClose(SDL_Event evt)
{
    switch (evt.type)
    {
        case SDL_QUIT:
            return true;
        case SDL_WINDOWEVENT:
            return evt.window.event == SDL_WINDOWEVENT_CLOSE;
        case SDL_KEYUP:
            return evt.key.keysym.sym == SDLK_ESCAPE;
        default:
            return false;
    }
}


int main()
{
    OpenGLWindow window(800, 600);

    // Scene initialization

    std::vector<float> quadMeshData = {
        -1, -1, 0, 0, 0,
        -1, 1, 0, 0, 1,
        1, 1, 0, 1, 1,
        1, -1, 0, 1, 0
    };

    std::vector<uint16_t> quadMeshIndices = {
        0, 1, 2,
        0, 2, 3
    };

    GLuint vertexBuffer = 0;
    glGenBuffers(1, &vertexBuffer);
    assert(vertexBuffer);

    GLuint indexBuffer = 0;
    glGenBuffers(1, &indexBuffer);
    assert(indexBuffer);

    GLuint vertexArray = 0;
    glGenVertexArrays(1, &vertexArray);
    assert(vertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 5 * 4, quadMeshData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 2 * 6, quadMeshIndices.data(), GL_STATIC_DRAW); // 2 because we currently support only UNSIGNED_SHORT indexes
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glBindVertexArray(vertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, reinterpret_cast<void *>(sizeof(float) * 3));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    std::string vsSrc = R"(
        #version 330 core

        layout (location = 0) in vec4 position;
        layout (location = 1) in vec2 texCoord0;

        uniform mat4 worldViewProjMatrix;
        out vec2 uv0;

        void main()
        {
            gl_Position = worldViewProjMatrix * position;
            //gl_Position = position;
            uv0 = texCoord0;
        }
    )";

    std::string fsSrc = R"(
        #version 330 core

        in vec2 uv0;
        out vec4 fragColor;

        void main()
        {
            fragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )";

    auto vs = compileShader(GL_VERTEX_SHADER, vsSrc.c_str(), vsSrc.size());
    auto fs = compileShader(GL_FRAGMENT_SHADER, fsSrc.c_str(), fsSrc.size());
    auto program = linkProgram(vs, fs);

    glDetachShader(program, vs);
    glDeleteShader(vs);
    glDetachShader(program, fs);
    glDeleteShader(fs);

    glUseProgram(program);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0, 0.8, 0.8, 1);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_BLEND);

    Transform cameraTransform;
    cameraTransform.setLocalPosition({5, 5, 5});
    cameraTransform.lookAt({0, 0, 0}, {0, 1, 0});

    Transform transform;

    auto projMatrix = TransformMatrix::createPerspective(Degree(60), 800.0 / 600, 0.1f, 100.0f);
    auto viewMatrix = cameraTransform.getWorldMatrix();
    viewMatrix.invert();
    auto wvpMatrix = (projMatrix * viewMatrix) * transform.getWorldMatrix();
    
    auto uniform = glGetUniformLocation(program, "worldViewProjMatrix");
    glUniformMatrix4fv(uniform, 1, GL_FALSE, wvpMatrix.m);

    auto run = true;
    while (run)
    {
        SDL_Event evt;
        while (SDL_PollEvent(&evt))
        {
            if (run)
                run = !shouldClose(evt);
        }

        glViewport(0, 0, 800, 600);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);

        glBindVertexArray(vertexArray);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
            
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        SDL_GL_SwapWindow(window.getWindow());
    }

    glDeleteVertexArrays(1, &vertexArray);
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &indexBuffer);

    return 0;
}