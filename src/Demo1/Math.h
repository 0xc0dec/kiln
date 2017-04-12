/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/epsilon.hpp>

namespace math
{
    static constexpr float epsilon1 = 0.000001f;
    static constexpr float epsilon2 = 1.0e-37f;
    static constexpr float epsilon3 = 2e-37f;
    static constexpr float pi = 3.14159265358979323846f;
    static constexpr float piOver2 = 1.57079632679489661923f;

    static bool isZero(float value, float epsilon = epsilon1);
    static bool isZero(const glm::vec3 &v, float epsilon = epsilon1);
    static bool isIdentity(const glm::quat &q, float epsilon = epsilon1);
    static bool isUnit(const glm::vec3 &v, float epsilon = epsilon1);

    static bool areEqual(float first, float second, float epsilon);

    static auto degToRad(float degrees) -> float;
    static auto radToDeg(float radians) -> float;

    static auto clamp(float x, float lo, float hi) -> float;
}

inline bool math::isZero(float value, float epsilon)
{
    return fabs(value) <= epsilon;
}

inline bool math::isZero(const glm::vec3 &v, float epsilon = epsilon1)
{
    return glm::all(glm::epsilonEqual(v, glm::vec3(0), epsilon));
}

inline bool math::isIdentity(const glm::quat &q, float epsilon = epsilon1)
{
    return glm::all(glm::epsilonEqual(q, glm::quat(1, 0, 0, 0), epsilon));
}

inline bool math::isUnit(const glm::vec3 &v, float epsilon = epsilon1)
{
    return glm::all(glm::epsilonEqual(v, glm::vec3(1, 1, 1), epsilon));
}

inline bool math::areEqual(float first, float second, float epsilon)
{
    return fabs(first - second) <= epsilon;
}

inline auto math::degToRad(float degrees) -> float
{
    return degrees * 0.0174532925f;
}

inline auto math::radToDeg(float radians) -> float
{
    return radians * 57.29577951f;
}

inline auto math::clamp(float x, float lo, float hi) -> float
{
    return (x < lo) ? lo : ((x > hi) ? hi : x);
}
