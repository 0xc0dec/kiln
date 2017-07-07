/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

class Camera;

enum class TransformSpace
{
    Self,
    Parent,
    World
};

class Transform final
{
public:
    // Can be used by anyone interested if a transform has changed. Goes from 0 to MAX_UINT, then wraps back.
    auto getVersion() const -> uint32_t;

    auto setParent(Transform *parent) -> Transform&;
    auto getParent() const -> Transform*;

    auto getChild(uint32_t index) const -> Transform*;
    auto getChildrenCount() const -> uint32_t;
    auto clearChildren() -> Transform&;

    auto getLocalScale() const -> glm::vec3;

    auto getWorldRotation() const -> glm::quat;
    auto getLocalRotation() const -> glm::quat;

    auto getWorldPosition() const -> glm::vec3;
    auto getLocalPosition() const -> glm::vec3;

    auto getWorldUp() const -> glm::vec3;
    auto getLocalUp() const -> glm::vec3;

    auto getWorldDown() const -> glm::vec3;
    auto getLocalDown() const -> glm::vec3;

    auto getWorldLeft() const -> glm::vec3;
    auto getLocalLeft() const -> glm::vec3;

    auto getWorldRight() const -> glm::vec3;
    auto getLocalRight() const -> glm::vec3;

    auto getWorldForward() const -> glm::vec3;
    auto getLocalForward() const -> glm::vec3;

    auto getWorldBack() const -> glm::vec3;
    auto getLocalBack() const -> glm::vec3;

    auto translateLocal(const glm::vec3 &translation) -> Transform&;
    auto scaleLocal(const glm::vec3 &scale) -> Transform&;

    auto setLocalPosition(const glm::vec3 &position) -> Transform&;
    auto setLocalScale(const glm::vec3 &scale) -> Transform&;

    auto rotate(const glm::quat &rotation, TransformSpace space = TransformSpace::Self) -> Transform&;
    auto rotate(const glm::vec3 &axis, float angle, TransformSpace space = TransformSpace::Self) -> Transform&;

    auto setLocalRotation(const glm::quat &rotation) -> Transform&;
    auto setLocalRotation(const glm::vec3 &axis, float angle) -> Transform&;

    auto lookAt(const glm::vec3 &target, const glm::vec3 &up) -> Transform&;

    auto getMatrix() const -> glm::mat4;
    auto getWorldMatrix() const -> glm::mat4;
    auto getInvTransposedWorldMatrix() const -> glm::mat4;
    auto getWorldViewMatrix(const Camera &camera) const -> glm::mat4;
    auto getWorldViewProjMatrix(const Camera &camera) const -> glm::mat4;
    auto getInvTransposedWorldViewMatrix(const Camera &camera) const -> glm::mat4;

    auto transformPoint(const glm::vec3 &point) const -> glm::vec3;
    auto transformDirection(const glm::vec3 &direction) const -> glm::vec3;
    
private:
    uint32_t version = 0;
    mutable uint32_t dirtyFlags = ~0;

    Transform *parent = nullptr;
    std::vector<Transform *> children;

    glm::vec3 localPosition;
    glm::vec3 localScale{1, 1, 1};
    glm::quat localRotation;
    mutable glm::mat4 matrix;
    mutable glm::mat4 worldMatrix;
    mutable glm::mat4 invTransposedWorldMatrix;

    void setDirtyWithChildren(uint32_t flags);
    void setChildrenDirty(uint32_t flags);
};

inline auto Transform::getLocalPosition() const -> glm::vec3
{
    return localPosition;
}

inline auto Transform::getWorldPosition() const -> glm::vec3
{
    return glm::vec3(getWorldMatrix()[3]);
}

inline auto Transform::getWorldUp() const -> glm::vec3
{
    auto m = getWorldMatrix();
    return {-m[1][0], -m[1][1], -m[1][2]};
}

inline auto Transform::getLocalUp() const -> glm::vec3
{
    auto m = getMatrix();
    return {-m[1][0], -m[1][1], -m[1][2]};
}

inline auto Transform::getWorldDown() const -> glm::vec3
{
    auto m = getWorldMatrix();
    return {m[1][0], m[1][1], m[1][2]};
}

inline auto Transform::getLocalDown() const -> glm::vec3
{
    auto m = getMatrix();
    return {m[1][0], m[1][1], m[1][2]};
}

inline auto Transform::getWorldLeft() const -> glm::vec3
{
    auto m = getWorldMatrix();
    return {-m[0][0], -m[0][1], -m[0][2]};
}

inline auto Transform::getLocalLeft() const -> glm::vec3
{
    auto m = getMatrix();
    return {-m[0][0], -m[0][1], -m[0][2]};
}

inline auto Transform::getWorldRight() const -> glm::vec3
{
    auto m = getWorldMatrix();
    return {m[0][0], m[0][1], m[0][2]};
}

inline auto Transform::getLocalRight() const -> glm::vec3
{
    auto m = getMatrix();
    return {m[0][0], m[0][1], m[0][2]};
}

inline auto Transform::getWorldForward() const -> glm::vec3
{
    auto m = getWorldMatrix();
    return {-m[2][0], -m[2][1], -m[2][2]};
}

inline auto Transform::getLocalForward() const -> glm::vec3
{
    auto m = getMatrix();
    return {-m[2][0], -m[2][1], -m[2][2]};
}

inline auto Transform::getWorldBack() const -> glm::vec3
{
    auto m = getWorldMatrix();
    return {m[2][0], m[2][1], m[2][2]};
}

inline auto Transform::getLocalBack() const -> glm::vec3
{
    auto m = getMatrix();
    return {m[2][0], m[2][1], m[2][2]};
}

inline auto Transform::getChildrenCount() const -> uint32_t
{
    return children.size();
}

inline auto Transform::getLocalScale() const -> glm::vec3
{
    return localScale;
}

inline auto Transform::getWorldRotation() const -> glm::quat
{
    return glm::quat_cast(getWorldMatrix());
}

inline auto Transform::getLocalRotation() const -> glm::quat
{
    return localRotation;
}

inline auto Transform::getParent() const -> Transform*
{
    return parent;
}

inline auto Transform::getChild(uint32_t index) const -> Transform*
{
    return children[index];
}

inline auto Transform::getVersion() const -> uint32_t
{
    return version;
}