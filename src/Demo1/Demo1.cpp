/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "OpenGLWindow.h"
#include "OpenGL.h"
#include "FileSystem.h"
#include "Image.h"
#include <SDL.h>
#include <vector>
#include <cassert>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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


static auto getWvpMatrix(float time) -> glm::mat4
{
    auto projMat = glm::perspective(glm::degrees(60.0f), 800.0f / 600, 0.1f, 100.0f);
    auto viewMat = glm::lookAt(glm::vec3(glm::sin(time) * 5.0f, glm::sin(time + 1) * 5.0f, 5.0f), glm::vec3(0, 0, 0), glm::vec3(0.0f, 1.0f, 0.0f));
    return projMat * viewMat;
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

    auto program = gl::createShaderProgram(vsSrc.c_str(), vsSrc.size(), fsSrc.c_str(), fsSrc.size());
    glUseProgram(program);

    auto imageBytes = fs::readBytes("../../../assets/Freeman.png");
    auto image = img::loadPNG(imageBytes);

    GLuint texture = 0;
    glGenTextures(1, &texture);
    assert(texture);

    glBindTexture(GL_TEXTURE_2D, texture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width, image.height, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);

    glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
    glGenerateMipmap(GL_TEXTURE_2D);

    auto wvpUniform = glGetUniformLocation(program, "worldViewProjMatrix");

    auto texUniform = glGetUniformLocation(program, "mainTex");
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(texUniform, 0);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0, 0.8, 0.8, 1);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_BLEND);
    glViewport(0, 0, 800, 600);

    window.loop([=](auto dt, auto time)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto wvpMat = getWvpMatrix(time);
        glUniformMatrix4fv(wvpUniform, 1, GL_FALSE, glm::value_ptr(wvpMat));

        glBindVertexArray(vertexArray);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    });

    glDeleteVertexArrays(1, &vertexArray);
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &indexBuffer);

    glDeleteTextures(1, &texture);

    return 0;
}