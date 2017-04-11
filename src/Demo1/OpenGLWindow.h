#pragma once

#include <SDL.h>

class OpenGLWindow final
{
public:
    OpenGLWindow(uint32_t canvasWidth, uint32_t canvasHeight);
    ~OpenGLWindow();

    auto getWindow() const -> SDL_Window*;

private:
    SDL_Window* window = nullptr;
    SDL_GLContext context = nullptr;
};

inline auto OpenGLWindow::getWindow() const -> SDL_Window*
{
    return window;
}