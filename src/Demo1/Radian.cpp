/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Radian.h"
#include "Degree.h"
#include "Math.h"


Radian::Radian(const Degree &d):
    raw(d.toRawRadian())
{
}


auto Radian::operator=(const Degree &d) -> Radian &
{
    raw = d.toRawRadian();
    return *this;
}


auto Radian::toRawDegree() const -> float
{
    return math::radToDeg(raw);
}


auto Radian::operator+(const Degree &d) const -> Radian
{
    return Radian(raw + d.toRawRadian());
}


auto Radian::operator+=(const Degree &d) -> Radian &
{
    raw += d.toRawRadian();
    return *this;
}


auto Radian::operator-(const Degree &d) const -> Radian
{
    return Radian(raw - d.toRawRadian());
}


auto Radian::operator-=(const Degree &d) -> Radian &
{
    raw -= d.toRawRadian();
    return *this;
}
