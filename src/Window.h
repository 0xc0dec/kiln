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

    auto getPlatformHandle() const -> std::vector<uint8_t>;

    void beginUpdate(Input &input);
    void endUpdate();

    bool closeRequested() const { return _closeRequested; }

    auto getTimeDelta() const -> float { return dt; }

private:
    SDL_Window *window = nullptr;
    
    float dt = 0;
    bool _closeRequested = false;
};

