/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include <string>
#include <vector>

namespace fs
{
    auto readBytes(const std::string &path) -> std::vector<uint8_t>;
}