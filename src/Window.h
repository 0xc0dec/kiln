/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "vulkan/Vulkan.h"

struct SDL_Window;
class Input;

class Window
{
public:
    Window(uint32_t canvasWidth, uint32_t canvasHeight, const char *title);
    Window(const Window &other) = delete;
    Window(Window &&other) = delete;
    ~Window();

    auto operator=(const Window &other) -> Window& = delete;
    auto operator=(Window &&other) -> Window& = delete;

    void beginUpdate(Input &input);
    void endUpdate();

    bool closeRequested() const;

    auto getTimeDelta() const -> float;

    auto getInstance() const -> VkInstance;
    auto getSurface() const -> VkSurfaceKHR;

private:
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

inline auto Window::getInstance() const -> VkInstance
{
    return instance;
}

inline auto Window::getSurface() const -> VkSurfaceKHR
{
    return surface;
}
