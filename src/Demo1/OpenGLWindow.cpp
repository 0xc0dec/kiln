/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "OpenGLWindow.h"
#include <GL/glew.h>


static bool shouldClose(SDL_Event evt)
{
    switch (evt.type)
    {
        case SDL_QUIT:
            return true;
        case SDL_WINDOWEVENT:
            return evt.window.event == SDL_WINDOWEVENT_CLOSE;
        case SDL_KEYUP:
            return evt.key.keysym.sym == SDLK_ESCAPE;
        default:
            return false;
    }
}


OpenGLWindow::OpenGLWindow(uint32_t canvasWidth, uint32_t canvasHeight)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        canvasWidth, canvasHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    context = SDL_GL_CreateContext(window);

    glewExperimental = true;
    if (glewInit() != GLEW_OK)
        cleanup(); // TODO logging

    SDL_GL_SetSwapInterval(1);
}


OpenGLWindow::~OpenGLWindow()
{
    cleanup();
}


void OpenGLWindow::loop(std::function<void(float, float)> update)
{
    auto run = true;
    auto lastTime = 0.0f;

    while (run)
    {
        SDL_Event evt;
        while (SDL_PollEvent(&evt))
        {
            if (run)
                run = !shouldClose(evt);
        }

        auto time = SDL_GetTicks() / 1000.0f;
        auto dt = time - lastTime;
        lastTime = time;

        update(dt, time);

        SDL_GL_SwapWindow(window);
    }
}


void OpenGLWindow::cleanup()
{
    if (context)
    {
        SDL_GL_DeleteContext(context);
        context = nullptr;
    }
    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}
