/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Camera.h"
#include "Radian.h"


const uint32_t ViewDirtyBit = 1;
const uint32_t ProjectionDirtyBit = 1 << 1;
const uint32_t ViewProjectionDirtyBit = 1 << 2;
const uint32_t InvViewDirtyBit = 1 << 3;
const uint32_t InvViewProjectionDirtyBit = 1 << 4;
const uint32_t AllProjectionDirtyBits = ProjectionDirtyBit | ViewProjectionDirtyBit | InvViewProjectionDirtyBit;


void Camera::setPerspective(bool perspective)
{
    ortho = !perspective;
    dirtyFlags |= AllProjectionDirtyBits;
}


void Camera::setFOV(const Radian &fov)
{
    this->fov = fov;
    dirtyFlags |= AllProjectionDirtyBits;
}


void Camera::setOrthoSize(const Vector2 &size)
{
    orthoSize = size;
    dirtyFlags |= AllProjectionDirtyBits;
}


void Camera::setAspectRatio(float ratio)
{
    aspectRatio = ratio;
    dirtyFlags |= AllProjectionDirtyBits;
}


void Camera::setFarZ(float far)
{
    this->farClip = far;
    dirtyFlags |= AllProjectionDirtyBits;
}


void Camera::setNearZ(float near)
{
    this->nearClip = near;
    dirtyFlags |= AllProjectionDirtyBits;
}


auto Camera::getViewMatrix() const -> const TransformMatrix
{
    if (dirtyFlags & ViewDirtyBit)
    {
        viewMatrix = transform.getWorldMatrix();
        viewMatrix.invert();
        dirtyFlags &= ~ViewDirtyBit;
    }
    return viewMatrix;
}


auto Camera::getInvViewMatrix() const -> const TransformMatrix
{
    if (dirtyFlags & InvViewDirtyBit)
    {
        invViewMatrix = getViewMatrix();
        invViewMatrix.invert();
        dirtyFlags &= ~InvViewDirtyBit;
    }
    return invViewMatrix;
}


auto Camera::getProjectionMatrix() const -> const TransformMatrix
{
    if (dirtyFlags & ProjectionDirtyBit)
    {
        if (ortho)
            projectionMatrix = TransformMatrix::createOrthographic(orthoSize.x, orthoSize.y, nearClip, farClip);
        else
            projectionMatrix = TransformMatrix::createPerspective(fov, aspectRatio, nearClip, farClip);
        dirtyFlags &= ~ProjectionDirtyBit;
    }
    return projectionMatrix;
}


auto Camera::getViewProjectionMatrix() const -> const TransformMatrix
{
    if (dirtyFlags & ViewProjectionDirtyBit)
    {
        viewProjectionMatrix = getProjectionMatrix() * getViewMatrix();
        dirtyFlags &= ~ViewProjectionDirtyBit;
    }
    return viewProjectionMatrix;
}


auto Camera::getInvViewProjectionMatrix() const -> const TransformMatrix
{
    if (dirtyFlags & InvViewProjectionDirtyBit)
    {
        invViewProjectionMatrix = getViewProjectionMatrix();
        invViewProjectionMatrix.invert();
        dirtyFlags &= ~InvViewProjectionDirtyBit;
    }
    return invViewProjectionMatrix;
}
