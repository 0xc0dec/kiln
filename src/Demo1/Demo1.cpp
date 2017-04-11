/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "OpenGLWindow.h"
#include <SDL.h>


bool shouldClose(SDL_Event evt)
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


int main()
{
    OpenGLWindow window(800, 600);

    auto run = true;
    while (run)
    {
        SDL_Event evt;
        while (SDL_PollEvent(&evt))
        {
            if (run)
                run = !shouldClose(evt);
        }

        SDL_GL_SwapWindow(window.getWindow());
    }

    return 0;
}