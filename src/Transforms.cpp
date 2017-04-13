/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Common/FileSystem.h"
#include "Common/Image.h"
#include "Common/OpenGLWindow.h"
#include "Common/OpenGL.h"
#include "Common/Camera.h"
#include "Common/Spectator.h"
#include <SDL.h>
#include <GL/glew.h>
#include <cassert>
#include <glm/gtc/type_ptr.hpp>


static const std::string vsSrc = R"(
    #version 330 core

    layout (location = 0) in vec4 position;
    layout (location = 1) in vec2 texCoord0;

    uniform mat4 worldViewProjMatrix;
    out vec2 uv0;

    void main()
    {
        gl_Position = worldViewProjMatrix * position;
        uv0 = texCoord0;
    }
)";

static const std::string fsSrc = R"(
    #version 330 core

    uniform sampler2D mainTex;

    in vec2 uv0;
    out vec4 fragColor;

    void main()
    {
        fragColor = texture(mainTex, uv0);
    }
)";


static const std::vector<float> vertices =
{
    -1, -1, 0, 0, 0,
    -1, 1, 0, 0, 1,
    1, 1, 0, 1, 1,
    1, -1, 0, 1, 0
};

static const std::vector<uint16_t> indices =
{
    0, 1, 2,
    0, 2, 3
};


static auto initVertexArray(GLuint vertexBuffer) -> GLuint
{
    GLuint handle = 0;
    glGenVertexArrays(1, &handle);
    assert(handle);

    glBindVertexArray(handle);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, reinterpret_cast<void *>(sizeof(float) * 3));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    return handle;
}


static auto initTexture(const img::Image &image) -> GLuint
{
    GLuint handle = 0;
    glGenTextures(1, &handle);
    assert(handle);

    glBindTexture(GL_TEXTURE_2D, handle);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width, image.height, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);

    glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    return handle;
}


// TODO more complex transform hierarchies
int main()
{
    OpenGLWindow window(800, 600);

    // Shader
    auto shaderProgram = gl::createShaderProgram(vsSrc.c_str(), vsSrc.size(), fsSrc.c_str(), fsSrc.size());
    glUseProgram(shaderProgram);

    // Texture
    auto imageBytes = fs::readBytes("../../assets/Freeman.png");
    auto image = img::loadPNG(imageBytes);
    auto texture = initTexture(image);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(shaderProgram, "mainTex"), 0);

    // Mesh
    auto vertexBuffer = gl::createVertexBuffer(vertices.data(), 4, 5);
    auto indexBuffer = gl::createIndexBuffer(indices.data(), 6);
    auto vertexArray = initVertexArray(vertexBuffer);
    glBindVertexArray(vertexArray);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

    // Some pipeline configuration
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0, 0.8, 0.8, 1);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_BLEND);
    glViewport(0, 0, 800, 600);

    Camera cam;
    cam.setPerspective(glm::degrees(60.0f), 800.0f / 600, 0.1f, 100.0f)
       .getTransform().setLocalPosition({4, 4, 10}).lookAt({0, 0, 0}, {0, 1, 0});

    Transform meshTransform;

    auto matrixUniform = glGetUniformLocation(shaderProgram, "worldViewProjMatrix");

    window.loop([&](auto dt, auto time)
    {
        updateSpectator(cam.getTransform(), window.getInput(), dt);

        auto matrix = meshTransform.rotate({1, 0, 0}, dt).getWorldViewProjMatrix(cam);
        glUniformMatrix4fv(matrixUniform, 1, GL_FALSE, glm::value_ptr(matrix));

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    });

    glDeleteVertexArrays(1, &vertexArray);
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &indexBuffer);
    glDeleteTextures(1, &texture);
    glDeleteProgram(shaderProgram);

    return 0;
}