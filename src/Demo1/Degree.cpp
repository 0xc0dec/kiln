/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Degree.h"
#include "Radian.h"
#include "Math.h"


Degree::Degree(const Radian &r):
    raw(r.toRawDegree())
{
}


auto Degree::operator=(const Radian &r) -> Degree &
{
    raw = r.toRawDegree();
    return *this;
}


auto Degree::toRawRadian() const -> float
{
    return math::degToRad(raw);
}


auto Degree::operator+(const Radian &r) const -> Degree
{
    return Degree(raw + r.toRawDegree());
}


auto Degree::operator+=(const Radian &r) -> Degree &
{
    raw += r.toRawDegree();
    return *this;
}


auto Degree::operator-(const Radian &r) const -> Degree
{
    return Degree(raw - r.toRawDegree());
}


auto Degree::operator-=(const Radian &r) -> Degree &
{
    raw -= r.toRawDegree();
    return *this;
}
