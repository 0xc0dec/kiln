/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Transform.h"
#include "TransformMatrix.h"
#include <glm/trigonometric.hpp>
#include <cstdint>

class Camera final
{
public:
    auto getTransform() -> Transform&;

    auto getViewport() const -> glm::vec4;
    void setViewport(const glm::vec4 &rect);

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

    auto getViewMatrix() const -> const TransformMatrix;
    auto getInvViewMatrix() const -> const TransformMatrix;
    auto getProjectionMatrix() const -> const TransformMatrix;
    auto getViewProjectionMatrix() const -> const TransformMatrix;
    auto getInvViewProjectionMatrix() const -> const TransformMatrix;

protected:
    glm::vec4 viewport;
    bool ortho = false;
    float fov = glm::degrees(60);
    glm::vec2 orthoSize{1, 1};
    float nearClip = 1;
    float farClip = 100;
    float aspectRatio = 1;

    Transform transform;

    mutable uint32_t dirtyFlags = ~0;

    mutable TransformMatrix viewMatrix;
    mutable TransformMatrix projectionMatrix;
    mutable TransformMatrix viewProjectionMatrix;
    mutable TransformMatrix invViewMatrix;
    mutable TransformMatrix invViewProjectionMatrix;
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

inline auto Camera::getViewport() const -> glm::vec4
{
    return viewport;
}

inline void Camera::setViewport(const glm::vec4 &rect)
{
    viewport = rect;
}

inline auto Camera::getTransform() -> Transform&
{
    return transform;
}

inline auto Camera::getOrthoSize() const -> glm::vec2
{
    return orthoSize;
}