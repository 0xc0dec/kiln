/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Ray.h"
#include "Math.h"
#include <algorithm>
#include <cassert>


Ray::Ray(const Vector3 &origin, const Vector3 &direction):
    origin(origin),
    direction(direction)
{
    normalize();
}


Ray::Ray(float originX, float originY, float originZ, float dirX, float dirY, float dirZ):
    origin(originX, originY, originZ),
    direction(dirX, dirY, dirZ)
{
    normalize();
}


void Ray::setOrigin(const Vector3 &origin)
{
    this->origin = origin;
}


void Ray::setDirection(const Vector3 &direction)
{
    this->direction = direction;
    normalize();
}


void Ray::normalize()
{
    assert(!direction.isZero());

    // Normalize the ray's direction vector
    auto normalizeFactor = 1.0f / sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    if (!math::areEqual(normalizeFactor, 1.0f, math::epsilon1))
    {
        direction.x *= normalizeFactor;
        direction.y *= normalizeFactor;
        direction.z *= normalizeFactor;
    }
}
