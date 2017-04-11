#include "OpenGLWindow.h"
#include <GL/glew.h>


OpenGLWindow::OpenGLWindow(uint32_t canvasWidth, uint32_t canvasHeight)
{
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
