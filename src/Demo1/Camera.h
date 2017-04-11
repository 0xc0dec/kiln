/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vector4.h"
#include "Transform.h"
#include "TransformMatrix.h"
#include "Radian.h"
#include "Degree.h"
#include <cstdint>

class Camera final
{
public:
    auto getTransform() -> Transform&;

    auto getViewport() const -> Vector4;
    void setViewport(const Vector4& rect);

    bool isPerspective() const;
    void setPerspective(bool perspective);

    auto getNear() const -> float;
    void setNear(float near);

    auto getFar() const -> float;
    void setFar(float far);

    auto getFOV() const -> Radian;
    void setFOV(const Radian& fov);

    auto getWidth() const -> float;
    void setWidth(float width);

    auto getHeight() const -> float;
    void setHeight(float height);

    auto getAspectRatio() const -> float;
    void setAspectRatio(float ratio);

    auto getViewMatrix() const -> const TransformMatrix;
    auto getInvViewMatrix() const -> const TransformMatrix;
    auto getProjectionMatrix() const -> const TransformMatrix;
    auto getViewProjectionMatrix() const -> const TransformMatrix;
    auto getInvViewProjectionMatrix() const -> const TransformMatrix;

protected:
    Vector4 viewport;
    bool ortho = false;
    Radian fov = Degree(60);
    float nearClip = 1;
    float farClip = 100;
    float width = 1;
    float height = 1;
    float aspectRatio = 1;

    Transform transform;

    mutable uint32_t transformDirtyFlags = ~0;

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

inline auto Camera::getNear() const -> float
{
    return nearClip;
}

inline auto Camera::getFar() const -> float
{
    return farClip;
}

inline auto Camera::getFOV() const -> Radian
{
    return fov;
}

inline auto Camera::getWidth() const -> float
{
    return width;
}

inline auto Camera::getHeight() const -> float
{
    return height;
}

inline auto Camera::getAspectRatio() const -> float
{
    return aspectRatio;
}

inline auto Camera::getViewport() const -> Vector4
{
    return viewport;
}

inline void Camera::setViewport(const Vector4& rect)
{
    viewport = rect;
}

inline auto Camera::getTransform() -> Transform&
{
    return transform;
}
