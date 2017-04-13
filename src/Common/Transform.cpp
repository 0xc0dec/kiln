/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Transform.h"
#include "Camera.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.inl>
#include <algorithm>


static const uint32_t DirtyBitLocal = 1;
static const uint32_t DirtyBitWorld = 1 << 1;
static const uint32_t DirtyBitInvTransposedWorld = 1 << 2;
static const uint32_t DirtyBitAll = DirtyBitLocal | DirtyBitWorld | DirtyBitInvTransposedWorld;


auto Transform::setParent(Transform *parent) -> Transform&
{
    if (parent != this && parent != this->parent)
    {
        if (this->parent)
        {
            auto &parentChildren = this->parent->children;
            this->parent->children.erase(std::remove(parentChildren.begin(), parentChildren.end(), this), parentChildren.end());
        }
        this->parent = parent;
        if (parent)
            parent->children.push_back(this);
        setDirtyWithChildren(DirtyBitWorld | DirtyBitInvTransposedWorld);
    }

    return *this;
}


auto Transform::clearChildren() -> Transform&
{
    while (!children.empty())
    {
        auto child = *children.begin();
        child->setParent(nullptr);
    }

    return *this;
}


auto Transform::getMatrix() const -> glm::mat4
{
    if (dirtyFlags & DirtyBitLocal)
    {
        matrix = glm::translate(glm::mat4(1.0f), localPosition);
        matrix = glm::rotate(matrix, glm::angle(localRotation), glm::axis(localRotation));
        matrix = glm::scale(matrix, localScale);

        dirtyFlags &= ~DirtyBitLocal;
    }

    return matrix;
}


auto Transform::getWorldMatrix() const -> glm::mat4
{
    if (dirtyFlags & DirtyBitWorld)
    {
        if (parent)
            worldMatrix = parent->getWorldMatrix() * getMatrix();
        else
            worldMatrix = getMatrix();
        dirtyFlags &= ~DirtyBitWorld;
    }

    return worldMatrix;
}


auto Transform::getInvTransposedWorldMatrix() const -> glm::mat4
{
    if (dirtyFlags & DirtyBitInvTransposedWorld)
    {
        invTransposedWorldMatrix = getWorldMatrix();
        invTransposedWorldMatrix = glm::transpose(glm::inverse(invTransposedWorldMatrix));
        dirtyFlags &= ~DirtyBitInvTransposedWorld;
    }

    return invTransposedWorldMatrix;
}


auto Transform::getWorldViewMatrix(const Camera &camera) const -> glm::mat4
{
    return camera.getViewMatrix() * getWorldMatrix();
}


auto Transform::getWorldViewProjMatrix(const Camera &camera) const -> glm::mat4
{
    return camera.getViewProjectionMatrix() * getWorldMatrix();
}


auto Transform::getInvTransposedWorldViewMatrix(const Camera &camera) const -> glm::mat4
{
    return glm::transpose(glm::inverse(camera.getViewMatrix() * getWorldMatrix()));
}


auto Transform::translateLocal(const glm::vec3 &translation) -> Transform&
{
    localPosition += translation;
    setDirtyWithChildren(DirtyBitAll);
    return *this;
}


auto Transform::rotate(const glm::quat &rotation, TransformSpace space) -> Transform&
{
    auto normalizedRotation = glm::normalize(rotation);

    switch (space)
    {
        case TransformSpace::Self:
            localRotation = localRotation * normalizedRotation;
            break;
        case TransformSpace::Parent:
            localRotation = normalizedRotation * localRotation;
            break;
        case TransformSpace::World:
        {
            auto invWorldRotation = glm::inverse(getWorldRotation());
            localRotation = localRotation * invWorldRotation * normalizedRotation * getWorldRotation();
            break;
        }
        default:
            break;
    }

    setDirtyWithChildren(DirtyBitAll);

    return *this;
}


auto Transform::rotate(const glm::vec3 &axis, float angle, TransformSpace space) -> Transform&
{
    auto rotation = glm::angleAxis(angle, axis);
    rotate(rotation, space);
    return *this;
}


auto Transform::scaleLocal(const glm::vec3 &scale) -> Transform&
{
    localScale.x *= scale.x;
    localScale.y *= scale.y;
    localScale.z *= scale.z;
    setDirtyWithChildren(DirtyBitAll);
    return *this;
}


auto Transform::setLocalScale(const glm::vec3 &scale) -> Transform&
{
    localScale = scale;
    setDirtyWithChildren(DirtyBitAll);
    return *this;
}


auto Transform::lookAt(const glm::vec3 &target, const glm::vec3 &up) -> Transform&
{
    auto localTarget = glm::vec4(target, 1);
    auto localUp = glm::vec4(up, 0);

    if (parent)
    {
        auto m = glm::inverse(parent->getWorldMatrix());
        localTarget = m * localTarget;
        localUp = m * localUp;
    }

    auto lookAtMatrix = glm::inverse(glm::lookAt(localPosition, glm::vec3(localTarget), glm::vec3(localUp)));
    setLocalRotation(glm::quat_cast(lookAtMatrix));

    return *this;
}


auto Transform::transformPoint(const glm::vec3 &point) const -> glm::vec3
{
    return getMatrix() * glm::vec4(point, 1.0f);
}


auto Transform::transformDirection(const glm::vec3 &direction) const -> glm::vec3
{
    return getMatrix() * glm::vec4(direction, 0);
}


auto Transform::setLocalRotation(const glm::quat &rotation) -> Transform&
{
    localRotation = rotation;
    setDirtyWithChildren(DirtyBitAll);
    return *this;
}


auto Transform::setLocalRotation(const glm::vec3 &axis, float angle) -> Transform&
{
    localRotation = glm::angleAxis(angle, axis);
    setDirtyWithChildren(DirtyBitAll);
    return *this;
}


auto Transform::setLocalPosition(const glm::vec3 &position) -> Transform&
{
    localPosition = position;
    setDirtyWithChildren(DirtyBitAll);
    return *this;
}


void Transform::setDirtyWithChildren(uint32_t flags)
{
    version++;
    dirtyFlags |= flags;
    for (auto child : children)
        child->setDirtyWithChildren(flags);
}


void Transform::setChildrenDirty(uint32_t flags)
{
    for (auto child : children)
        child->setDirtyWithChildren(flags);
}
