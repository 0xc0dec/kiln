/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "OpenGLWindow.h"
#include "Transform.h"
#include "Degree.h"
#include "OpenGL.h"
#include <SDL.h>
#include <GL/glew.h>
#include <vector>
#include <cassert>


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

    auto vertexBuffer = gl::createVertexBuffer(quadMeshData.data(), 4, 5);
    auto indexBuffer = gl::createIndexBuffer(quadMeshIndices.data(), 6);
    
    GLuint vertexArray = 0;
    glGenVertexArrays(1, &vertexArray);
    assert(vertexArray);

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

    auto program = gl::createShaderProgram(vsSrc.c_str(), vsSrc.size(), fsSrc.c_str(), fsSrc.size());
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