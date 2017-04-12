/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Transform.h"
#include "Camera.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.inl>
#include <algorithm>
#include "Math.h"


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
    auto dirty = dirtyFlags & TransformDirtyFlags::Position ||
                 dirtyFlags & TransformDirtyFlags::Rotation ||
                 dirtyFlags & TransformDirtyFlags::Scale;
    if (dirty)
    {
        if (dirtyFlags & TransformDirtyFlags::Position || !math::isZero(localPosition))
        {
            matrix = glm::translate(glm::mat4(1.0f), localPosition);
            if (dirtyFlags & TransformDirtyFlags::Rotation || !math::isIdentity(localRotation))
                matrix = glm::rotate(matrix, glm::angle(localRotation), glm::axis(localRotation));
            if (dirtyFlags & TransformDirtyFlags::Scale || !math::isUnit(localScale))
                matrix = glm::scale(matrix, localScale);
        }
        else if (dirtyFlags & TransformDirtyFlags::Rotation || !math::isIdentity(localRotation))
        {
            matrix = glm::rotate(glm::mat4(1.0f), glm::angle(localRotation), glm::axis(localRotation));
            if (dirtyFlags & TransformDirtyFlags::Scale || !math::isUnit(localScale))
                matrix = glm::scale(matrix, localScale);
        }
        else if (dirtyFlags & TransformDirtyFlags::Scale || !math::isUnit(localScale))
            matrix = glm::scale(glm::mat4(1.0f), localScale);

        dirtyFlags &= ~(TransformDirtyFlags::Position | TransformDirtyFlags::Rotation | TransformDirtyFlags::Scale);
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
    auto result = camera.getViewMatrix() * getWorldMatrix();
    result.invert();
    result.transpose();
    return result;
}


void Transform::translateLocal(const glm::vec3 &translation)
{
    localPosition += translation;
    setDirtyWithChildren(TransformDirtyFlags::Position | TransformDirtyFlags::World | TransformDirtyFlags::InvTransposedWorld);
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

    setDirtyWithChildren(TransformDirtyFlags::Rotation | TransformDirtyFlags::World | TransformDirtyFlags::InvTransposedWorld);
}


void Transform::rotateByAxisAngle(const glm::vec3 &axis, float angle, TransformSpace space)
{
    auto rotation = glm::angleAxis(angle, axis);
    rotate(rotation, space);
}


void Transform::scaleLocal(const Vector3 &scale)
{
    localScale.x *= scale.x;
    localScale.y *= scale.y;
    localScale.z *= scale.z;
    setDirtyWithChildren(TransformDirtyFlags::Scale | TransformDirtyFlags::World | TransformDirtyFlags::InvTransposedWorld);
}


void Transform::setLocalScale(const Vector3 &scale)
{
    localScale = scale;
    setDirtyWithChildren(TransformDirtyFlags::Scale | TransformDirtyFlags::World | TransformDirtyFlags::InvTransposedWorld);
}


void Transform::lookAt(const Vector3 &target, const Vector3 &up)
{
    auto localTarget = target;
    auto localUp = up;

    if (parent)
    {
        auto m(parent->getWorldMatrix());
        m.invert();
        localTarget = m.transformPoint(target);
        localUp = m.transformDirection(up);
    }

    auto lookAtMatrix = TransformMatrix::createLookAt(localPosition, localTarget, localUp);
    setLocalRotation(lookAtMatrix.getRotation());
}


auto Transform::transformPoint(const Vector3 &point) const -> Vector3
{
    return getMatrix().transformPoint(point);
}


auto Transform::transformDirection(const Vector3 &direction) const -> Vector3
{
    return getMatrix().transformDirection(direction);
}


void Transform::setLocalRotation(const Quaternion &rotation)
{
    localRotation = rotation;
    setDirtyWithChildren(TransformDirtyFlags::Rotation | TransformDirtyFlags::World | TransformDirtyFlags::InvTransposedWorld);
}


void Transform::setLocalAxisAngleRotation(const Vector3 &axis, const Radian &angle)
{
    localRotation = Quaternion::createFromAxisAngle(axis, angle);
    setDirtyWithChildren(TransformDirtyFlags::Rotation | TransformDirtyFlags::World | TransformDirtyFlags::InvTransposedWorld);
}


void Transform::setLocalPosition(const Vector3 &position)
{
    localPosition = position;
    setDirtyWithChildren(TransformDirtyFlags::Position | TransformDirtyFlags::World | TransformDirtyFlags::InvTransposedWorld);
}


void Transform::setDirtyWithChildren(uint32_t flags) const
{
    dirtyFlags |= flags;
    for (auto child : children)
        child->setDirtyWithChildren(flags);
}


void Transform::setChildrenDirty(uint32_t flags) const
{
    for (auto child : children)
        child->setDirtyWithChildren(flags);
}
