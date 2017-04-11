#include <SDL.h>
#include <SDL_surface.h>
#include <GL/glew.h>

int main()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;

    auto window = SDL_CreateWindow("Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, flags);
    auto context = SDL_GL_CreateContext(window);

    glewExperimental = true;
    if (glewInit() != GLEW_OK)
    {
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    SDL_GL_SetSwapInterval(1);

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

        SDL_GL_SwapWindow(window);
    }

    // Shutdown
    if (context)
        SDL_GL_DeleteContext(context);
    if (window)
        SDL_DestroyWindow(window);

    return 0;
}