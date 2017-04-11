/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "FileSystem.h"
#include <fstream>
#include <cassert>


auto fs::readBytes(const std::string& path) -> std::vector<uint8_t>
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    assert(file.is_open());

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    auto data = std::vector<uint8_t>(size);
    file.read(reinterpret_cast<char *>(&data[0]), size);
    
    file.close();

    return data;
}
