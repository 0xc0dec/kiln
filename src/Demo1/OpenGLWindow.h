/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include <SDL.h>
#include <functional>

class OpenGLWindow final
{
public:
    OpenGLWindow(uint32_t canvasWidth, uint32_t canvasHeight);
    ~OpenGLWindow();

    void loop(std::function<void(float, float)> update);

private:
    SDL_Window* window = nullptr;
    SDL_GLContext context = nullptr;

    void cleanup();
};
