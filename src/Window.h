/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Common.h"
#include "Input.h"
#include "vulkan/Vulkan.h"

struct SDL_Window;

class Window
{
public:
    KL_DISABLE_COPY_AND_MOVE(Window)

    Window(uint32_t canvasWidth, uint32_t canvasHeight, const char *title);
    ~Window();

    void beginUpdate();
    void endUpdate();

    bool closeRequested() const;

    auto getTimeDelta() const -> float;

    auto getInput() -> Input&;
    auto getInstance() const -> VkInstance;
    auto getSurface() const -> VkSurfaceKHR;

private:
    Input input;
    SDL_Window *window = nullptr;
    vk::Resource<VkInstance> instance;
    vk::Resource<VkSurfaceKHR> surface;
    vk::Resource<VkDebugReportCallbackEXT> debugCallback;

    float dt = 0;
    bool _closeRequested = false;
};

inline auto Window::getTimeDelta() const -> float
{
    return dt;
}

inline bool Window::closeRequested() const
{
    return _closeRequested;
}

inline auto Window::getInput() -> Input&
{
    return input;
}

inline auto Window::getInstance() const -> VkInstance
{
    return instance;
}

inline auto Window::getSurface() const -> VkSurfaceKHR
{
    return surface;
}
