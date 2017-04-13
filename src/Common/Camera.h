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

    bool isPerspective() const;
    void setPerspective(bool perspective);

    auto getNearZ() const -> float;
    void setNearZ(float near);

    auto getFarZ() const -> float;
    void setFarZ(float far);

    auto getFOV() const -> float;
    void setFOV(float fov);

    auto getOrthoSize() const -> glm::vec2;
    void setOrthoSize(const glm::vec2 &size);

    auto getAspectRatio() const -> float;
    void setAspectRatio(float ratio);

    auto getViewMatrix() const -> const glm::mat4;
    auto getInvViewMatrix() const -> const glm::mat4;
    auto getProjectionMatrix() const -> const glm::mat4;
    auto getViewProjectionMatrix() const -> const glm::mat4;
    auto getInvViewProjectionMatrix() const -> const glm::mat4;

protected:
    bool ortho = false;
    float fov = glm::degrees(60.0f);
    glm::vec2 orthoSize{1, 1};
    float nearClip = 1;
    float farClip = 100;
    float aspectRatio = 1;

    Transform transform;
};

inline bool Camera::isPerspective() const
{
    return !ortho;
}

inline auto Camera::getNearZ() const -> float
{
    return nearClip;
}

inline auto Camera::getFarZ() const -> float
{
    return farClip;
}

inline auto Camera::getFOV() const -> float
{
    return fov;
}

inline auto Camera::getAspectRatio() const -> float
{
    return aspectRatio;
}

inline auto Camera::getTransform() -> Transform&
{
    return transform;
}

inline auto Camera::getOrthoSize() const -> glm::vec2
{
    return orthoSize;
}

inline void Camera::setPerspective(bool perspective)
{
    ortho = !perspective;
}

inline void Camera::setFOV(float fov)
{
    this->fov = fov;
}

inline void Camera::setOrthoSize(const glm::vec2 &size)
{
    orthoSize = size;
}

inline void Camera::setAspectRatio(float ratio)
{
    aspectRatio = ratio;
}

inline void Camera::setFarZ(float far)
{
    this->farClip = far;
}

inline void Camera::setNearZ(float near)
{
    this->nearClip = near;
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
