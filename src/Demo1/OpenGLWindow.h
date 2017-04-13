/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Input.h"
#include <SDL.h>
#include <functional>

class OpenGLWindow final
{
public:
    OpenGLWindow(uint32_t canvasWidth, uint32_t canvasHeight);
    ~OpenGLWindow();

    auto getInput() -> Input&;

    void loop(std::function<void(float, float)> update);

private:
    SDL_Window* window = nullptr;
    SDL_GLContext context = nullptr;
    Input input;

    void cleanup();
};

inline auto OpenGLWindow::getInput() -> Input&
{
    return input;
}