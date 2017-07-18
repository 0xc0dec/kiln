/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include <string>
#include <vector>
#include <functional>

namespace fs
{
    auto readBytes(const std::string &path) -> std::vector<uint8_t>;
    void iterateLines(const std::string &path, std::function<bool(const std::string &)> process);
}