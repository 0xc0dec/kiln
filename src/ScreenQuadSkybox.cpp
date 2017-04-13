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


static const char *meshVs = R"(
    #version 330 core

    layout (location = 0) in vec4 position;

    uniform mat4 projMatrix;
    uniform mat4 worldViewMatrix;

    void main()
    {
        gl_Position = projMatrix * worldViewMatrix * position;
    }
)";

static const char *meshFs = R"(
    #version 330 core

    out vec4 fragColor;

    void main()
    {
        fragColor = vec4(0.0, 1.0, 0.0, 1.0);
    }
)";

static const char *skyboxVs = R"(
    #version 330 core

    layout (location = 0) in vec4 position;

    uniform mat4 projMatrix;
    uniform mat4 worldViewMatrix;
    smooth out vec3 eyeDir;

    void main()
    {
        mat4 invProjMatrix = inverse(projMatrix);
        mat3 invModelViewMatrix = inverse(mat3(worldViewMatrix));
        vec3 unprojected = (invProjMatrix * position).xyz;
        eyeDir = invModelViewMatrix * unprojected;
        gl_Position = position;
    }
)";

static const char *skyboxFs = R"(
    #version 330 core

    uniform samplerCube mainTex;

    smooth in vec3 eyeDir;
    out vec4 fragColor;

    void main()
    {
        fragColor = texture(mainTex, eyeDir);
    }
)";


static const std::vector<float> quadVertices =
{
    -1, -1, 0, 0, 0,
    -1, 1, 0, 0, 1,
    1, 1, 0, 1, 1,
    1, -1, 0, 1, 0
};

static const std::vector<uint16_t> quadIndices =
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


static auto initTexture() -> GLuint
{
    auto frontImgBytes = fs::readBytes("../../assets/skyboxes/deep-space/Front.png");
    auto frontImg = img::loadPNG(frontImgBytes);

    auto backImgBytes = fs::readBytes("../../assets/skyboxes/deep-space/Back.png");
    auto backImg = img::loadPNG(backImgBytes);

    auto leftImgBytes = fs::readBytes("../../assets/skyboxes/deep-space/Left.png");
    auto leftImg = img::loadPNG(leftImgBytes);

    auto rightImgBytes = fs::readBytes("../../assets/skyboxes/deep-space/Right.png");
    auto rightImg = img::loadPNG(rightImgBytes);

    auto topImgBytes = fs::readBytes("../../assets/skyboxes/deep-space/Top.png");
    auto topImg = img::loadPNG(topImgBytes);

    auto bottomImgBytes = fs::readBytes("../../assets/skyboxes/deep-space/Bottom.png");
    auto bottomImg = img::loadPNG(bottomImgBytes);

    GLuint handle = 0;
    glGenTextures(1, &handle);
    assert(handle);

    glBindTexture(GL_TEXTURE_CUBE_MAP, handle);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, frontImg.width, frontImg.height, 0, GL_RGB, GL_UNSIGNED_BYTE, frontImg.data.data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, backImg.width, backImg.height, 0, GL_RGB, GL_UNSIGNED_BYTE, backImg.data.data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, leftImg.width, leftImg.height, 0, GL_RGB, GL_UNSIGNED_BYTE, leftImg.data.data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, rightImg.width, rightImg.height, 0, GL_RGB, GL_UNSIGNED_BYTE, rightImg.data.data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, topImg.width, topImg.height, 0, GL_RGB, GL_UNSIGNED_BYTE, topImg.data.data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, bottomImg.width, bottomImg.height, 0, GL_RGB, GL_UNSIGNED_BYTE, bottomImg.data.data());

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);

    glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return handle;
}


int main()
{
    OpenGLWindow window(800, 600);

    // Shaders
    auto skyboxShaderProgram = gl::createShaderProgram(skyboxVs, strlen(skyboxVs), skyboxFs, strlen(skyboxFs));
    glUseProgram(skyboxShaderProgram);

    auto quadShaderProgram = gl::createShaderProgram(meshVs, strlen(meshVs), meshFs, strlen(meshFs));

    // Texture
    auto texture = initTexture();
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(skyboxShaderProgram, "mainTex"), 0);

    // Mesh
    auto vertexBuffer = gl::createVertexBuffer(quadVertices.data(), 4, 5);
    auto indexBuffer = gl::createIndexBuffer(quadIndices.data(), 6);
    auto vertexArray = initVertexArray(vertexBuffer);
    glBindVertexArray(vertexArray);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

    // Some pipeline configuration
    glClearColor(0, 0.8, 0.8, 1);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_BLEND);
    glViewport(0, 0, 800, 600);

    Camera cam;
    cam.setPerspective(glm::degrees(60.0f), 800.0f / 600, 0.1f, 100.0f)
       .getTransform().setLocalPosition({4, 4, 10}).lookAt({0, 0, 0}, {0, 1, 0});

    glUseProgram(skyboxShaderProgram);
    auto skyboxProjMatrixUniform = glGetUniformLocation(skyboxShaderProgram, "projMatrix");
    auto skyboxWorldViewMatrixUniform = glGetUniformLocation(skyboxShaderProgram, "worldViewMatrix");

    glUseProgram(quadShaderProgram);
    auto quadProjMatrixUniform = glGetUniformLocation(quadShaderProgram, "projMatrix");
    auto quadWorldViewMatrixUniform = glGetUniformLocation(quadShaderProgram, "worldViewMatrix");

    window.loop([&](auto dt, auto time)
    {
        updateSpectator(cam.getTransform(), window.getInput(), dt);

        auto projMatrix = cam.getProjectionMatrix();
        auto worldViewMatrix = cam.getViewMatrix(); // world matrix is identity anyway
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);
        glUseProgram(skyboxShaderProgram);
        glUniformMatrix4fv(skyboxProjMatrixUniform, 1, GL_FALSE, glm::value_ptr(projMatrix));
        glUniformMatrix4fv(skyboxWorldViewMatrixUniform, 1, GL_FALSE, glm::value_ptr(worldViewMatrix));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glUseProgram(quadShaderProgram);
        glUniformMatrix4fv(quadProjMatrixUniform, 1, GL_FALSE, glm::value_ptr(projMatrix));
        glUniformMatrix4fv(quadWorldViewMatrixUniform, 1, GL_FALSE, glm::value_ptr(worldViewMatrix));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    });

    glDeleteVertexArrays(1, &vertexArray);
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &indexBuffer);
    glDeleteTextures(1, &texture);
    glDeleteProgram(skyboxShaderProgram);
    glDeleteProgram(quadShaderProgram);

    return 0;
}