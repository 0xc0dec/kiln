/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Transform.h"
#include <glm/glm.hpp>

class Camera final
{
public:
    auto getTransform() -> Transform&;

    auto setPerspective(float fov, float aspectRatio, float nearClip, float farClip) -> Camera&;
    auto setOrthographic(float width, float height, float nearClip, float farClip) -> Camera&;

    auto getViewMatrix() const -> const glm::mat4;
    auto getInvViewMatrix() const -> const glm::mat4;
    auto getProjectionMatrix() const -> const glm::mat4;
    auto getViewProjectionMatrix() const -> const glm::mat4;
    auto getInvViewProjectionMatrix() const -> const glm::mat4;

protected:
    bool ortho = false;
    float fov = glm::degrees(60.0f);
    float aspectRatio = 1;
    float orthoWidth = 1;
    float orthoHeight = 1;
    float nearClip = 1;
    float farClip = 100;
    
    Transform transform;
};

inline auto Camera::getTransform() -> Transform&
{
    return transform;
}

inline auto Camera::getViewMatrix() const -> const glm::mat4
{
    return glm::inverse(transform.getWorldMatrix());
}

inline auto Camera::getInvViewMatrix() const -> const glm::mat4
{
    return glm::inverse(getViewMatrix());
}

inline auto Camera::getViewProjectionMatrix() const -> const glm::mat4
{
    return getProjectionMatrix() * getViewMatrix();
}

inline auto Camera::getInvViewProjectionMatrix() const -> const glm::mat4
{
    return glm::inverse(getViewProjectionMatrix());
}
