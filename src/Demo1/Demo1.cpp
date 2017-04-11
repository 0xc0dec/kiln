#include "OpenGLWindow.h"
#include <SDL.h>


int main()
{
    OpenGLWindow window(800, 600);

    auto run = true;
    while (run)
    {
        SDL_Event evt;
        while (SDL_PollEvent(&evt))
        {
            switch (evt.type)
            {
                case SDL_QUIT:
                    run = false;
                    break;
                case SDL_WINDOWEVENT:
                    if (evt.window.event == SDL_WINDOWEVENT_CLOSE)
                        run = false;
                    break;
                case SDL_KEYUP:
                    if (evt.key.keysym.sym == SDLK_ESCAPE)
                        run = false;
                default:
                    break;
            }
        }

        SDL_GL_SwapWindow(window.getWindow());
    }

    return 0;
}