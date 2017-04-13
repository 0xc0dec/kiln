/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Transform.h"
#include "Camera.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.inl>
#include <algorithm>


struct TransformDirtyFlags final
{
    static const uint32_t Local = 1;
    static const uint32_t World = 1 << 1;
    static const uint32_t InvTransposedWorld = 1 << 2;
};


void Transform::setParent(Transform *parent)
{
    if (parent == this || parent == this->parent)
        return;
    if (this->parent)
    {
        auto &parentChildren = this->parent->children;
        this->parent->children.erase(std::remove(parentChildren.begin(), parentChildren.end(), this), parentChildren.end());
    }
    this->parent = parent;
    if (parent)
        parent->children.push_back(this);
    setDirtyWithChildren(TransformDirtyFlags::World | TransformDirtyFlags::InvTransposedWorld);
}


void Transform::clearChildren()
{
    while (!children.empty())
    {
        auto child = *children.begin();
        child->setParent(nullptr);
    }
}


auto Transform::getMatrix() const -> glm::mat4
{
    if (dirtyFlags & TransformDirtyFlags::Local)
    {
        matrix = glm::translate(glm::mat4(1.0f), localPosition);
        matrix = glm::rotate(matrix, glm::angle(localRotation), glm::axis(localRotation));
        matrix = glm::scale(matrix, localScale);

        dirtyFlags &= ~TransformDirtyFlags::Local;
    }

    return matrix;
}


auto Transform::getWorldMatrix() const -> glm::mat4
{
    if (dirtyFlags & TransformDirtyFlags::World)
    {
        if (parent)
            worldMatrix = parent->getWorldMatrix() * getMatrix();
        else
            worldMatrix = getMatrix();
        dirtyFlags &= ~TransformDirtyFlags::World;
    }

    return worldMatrix;
}


auto Transform::getInvTransposedWorldMatrix() const -> glm::mat4
{
    if (dirtyFlags & TransformDirtyFlags::InvTransposedWorld)
    {
        invTransposedWorldMatrix = getWorldMatrix();
        invTransposedWorldMatrix = glm::transpose(glm::inverse(invTransposedWorldMatrix));
        dirtyFlags &= ~TransformDirtyFlags::InvTransposedWorld;
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


void Transform::translateLocal(const glm::vec3 &translation)
{
    localPosition += translation;
    setDirtyWithChildren(TransformDirtyFlags::Local | TransformDirtyFlags::World | TransformDirtyFlags::InvTransposedWorld);
}


void Transform::rotate(const glm::quat &rotation, TransformSpace space)
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

    setDirtyWithChildren(TransformDirtyFlags::Local | TransformDirtyFlags::World | TransformDirtyFlags::InvTransposedWorld);
}


void Transform::rotateByAxisAngle(const glm::vec3 &axis, float angle, TransformSpace space)
{
    auto rotation = glm::angleAxis(angle, axis);
    rotate(rotation, space);
}


void Transform::scaleLocal(const glm::vec3 &scale)
{
    localScale.x *= scale.x;
    localScale.y *= scale.y;
    localScale.z *= scale.z;
    setDirtyWithChildren(TransformDirtyFlags::Local | TransformDirtyFlags::World | TransformDirtyFlags::InvTransposedWorld);
}


void Transform::setLocalScale(const glm::vec3 &scale)
{
    localScale = scale;
    setDirtyWithChildren(TransformDirtyFlags::Local | TransformDirtyFlags::World | TransformDirtyFlags::InvTransposedWorld);
}


void Transform::lookAt(const glm::vec3 &target, const glm::vec3 &up)
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
}


auto Transform::transformPoint(const glm::vec3 &point) const -> glm::vec3
{
    return getMatrix() * glm::vec4(point, 1.0f);
}


auto Transform::transformDirection(const glm::vec3 &direction) const -> glm::vec3
{
    return getMatrix() * glm::vec4(direction, 0);
}


void Transform::setLocalRotation(const glm::quat &rotation)
{
    localRotation = rotation;
    setDirtyWithChildren(TransformDirtyFlags::Local | TransformDirtyFlags::World | TransformDirtyFlags::InvTransposedWorld);
}


void Transform::setLocalAxisAngleRotation(const glm::vec3 &axis, float angle)
{
    localRotation = glm::angleAxis(angle, axis);
    setDirtyWithChildren(TransformDirtyFlags::Local | TransformDirtyFlags::World | TransformDirtyFlags::InvTransposedWorld);
}


void Transform::setLocalPosition(const glm::vec3 &position)
{
    localPosition = position;
    setDirtyWithChildren(TransformDirtyFlags::Local | TransformDirtyFlags::World | TransformDirtyFlags::InvTransposedWorld);
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
