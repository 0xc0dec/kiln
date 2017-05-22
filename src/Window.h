/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Common.h"
#include "vulkan/Vulkan.h"
#include <cstdint>

struct SDL_Window;

class Window
{
public:
    KL_DISABLE_COPY_AND_MOVE(Window)

    Window(uint32_t canvasWidth, uint32_t canvasHeight, const char *title);
    ~Window();

    auto getSdlWindow() const -> SDL_Window*;
    auto getInstance() const -> VkInstance;
    auto getSurface() const -> VkSurfaceKHR;

private:
    SDL_Window *window = nullptr;
    vk::Resource<VkInstance> instance;
    vk::Resource<VkSurfaceKHR> surface;
};

inline auto Window::getSdlWindow() const -> SDL_Window*
{
    return window;
}

inline auto Window::getInstance() const -> VkInstance
{
    return instance;
}

inline auto Window::getSurface() const -> VkSurfaceKHR
{
    return surface;
}
