/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Window.h"
#include "Input.h"
#include "Common.h"
#include <SDL.h>
#include <SDL_syswm.h>
#ifdef KL_WINDOWS
#   include <windows.h>
#endif
#include <vector>

Window::Window(uint32_t canvasWidth, uint32_t canvasHeight, const char *title)
{
    window = SDL_CreateWindow(title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        canvasWidth, canvasHeight,
        SDL_WINDOW_ALLOW_HIGHDPI);
}

Window::~Window()
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}

auto Window::getPlatformHandle() const -> std::vector<uint8_t>
{
#ifdef KL_WINDOWS
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);

    struct
    {
        HWND hWnd;
        HINSTANCE hInst;
    } handle{};

    handle.hWnd = wmInfo.info.win.window;
    handle.hInst = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(handle.hWnd, GWLP_HINSTANCE));

    std::vector<uint8_t> result;
    result.resize(sizeof(handle));
    memcpy(result.data(), &handle, sizeof(handle));

    return result;
#else
    KL_PANIC("Not implemented");
    return {};
#endif
}

void Window::beginUpdate(Input &input)
{
    static auto lastTicks = SDL_GetTicks();

    input.beginUpdate(window);

    SDL_Event evt;
    while (SDL_PollEvent(&evt))
    {
        input.processEvent(evt);

		const auto closeWindowEvent = evt.type == SDL_WINDOWEVENT && evt.window.event == SDL_WINDOWEVENT_CLOSE;
        if (evt.type == SDL_QUIT || closeWindowEvent)
            _closeRequested = true;
    }

    const auto ticks = SDL_GetTicks();
	const auto deltaTicks = ticks - lastTicks;
    if (deltaTicks > 0)
    {
        dt = deltaTicks / 1000.0f;
        lastTicks = ticks;
    }
}

void Window::endUpdate()
{
}
