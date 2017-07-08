/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Window.h"
#include <SDL.h>
#include <SDL_syswm.h>
#ifdef KL_WINDOWS
#   include <windows.h>
#endif
#include <vector>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackFunc(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
        uint64_t obj, size_t location, int32_t code, const char *layerPrefix, const char *msg, void *userData)
{
    // TODO do something here
    return VK_FALSE;
}

Window::Window(uint32_t canvasWidth, uint32_t canvasHeight, const char *title)
{
    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, canvasWidth, canvasHeight, SDL_WINDOW_ALLOW_HIGHDPI);

    VkApplicationInfo appInfo {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "";
    appInfo.pEngineName = "";
    appInfo.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char *> enabledExtensions {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef KL_WINDOWS
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#ifdef KL_DEBUG
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
    };

    std::vector<const char *> enabledLayers {
#ifdef KL_DEBUG
        "VK_LAYER_LUNARG_standard_validation",
#endif
    };

    VkInstanceCreateInfo instanceInfo {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext = nullptr;
    instanceInfo.pApplicationInfo = &appInfo;

    if (!enabledLayers.empty())
    {
        instanceInfo.enabledLayerCount = enabledLayers.size();
        instanceInfo.ppEnabledLayerNames = enabledLayers.data();
    }

    if (!enabledExtensions.empty())
    {
        instanceInfo.enabledExtensionCount = enabledExtensions.size();
        instanceInfo.ppEnabledExtensionNames = enabledExtensions.data();
    }

    KL_VK_CHECK_RESULT(vkCreateInstance(&instanceInfo, nullptr, instance.cleanRef()));

#ifdef KL_WINDOWS
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);

    auto hwnd = wmInfo.info.win.window;
    auto hinstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(hwnd, GWLP_HINSTANCE));

    VkWin32SurfaceCreateInfoKHR surfaceInfo;
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.flags = 0;
    surfaceInfo.pNext = nullptr;
    surfaceInfo.hinstance = hinstance;
    surfaceInfo.hwnd = hwnd;

    KL_VK_CHECK_RESULT(vkCreateWin32SurfaceKHR(instance, &surfaceInfo, nullptr, surface.cleanRef()));
#endif

#ifdef KL_DEBUG
    debugCallback = createDebugCallback(instance, debugCallbackFunc);
#endif
}

Window::~Window()
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Window::beginUpdate()
{
    static auto lastTicks = SDL_GetTicks();

    input.beginUpdate(window);

    SDL_Event evt;
    while (SDL_PollEvent(&evt))
    {
        input.processEvent(evt);

        if (evt.type == SDL_QUIT ||
            evt.type == SDL_WINDOWEVENT && evt.window.event == SDL_WINDOWEVENT_CLOSE ||
            evt.type == SDL_KEYUP && evt.key.keysym.sym == SDLK_ESCAPE)
        {
            _closeRequested = true;
        }
    }

    auto ticks = SDL_GetTicks();
    auto deltaTicks = ticks - lastTicks;
    if (deltaTicks == 0)
        deltaTicks = 1;

    dt = deltaTicks / 1000.0f;
    lastTicks = ticks;
}

void Window::endUpdate()
{
}
