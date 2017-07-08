/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include <vector>

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

    auto getPlatformHandle() -> std::vector<uint8_t>;

    void beginUpdate(Input &input);
    void endUpdate();

    bool closeRequested() const;

    auto getTimeDelta() const -> float;

private:
    SDL_Window *window = nullptr;
    
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
