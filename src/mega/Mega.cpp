/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "../Common/Input.h"
#include "Vulkan.h"

#include <SDL.h>
#include <SDL_syswm.h>
#include <windows.h>
#include <vector>

int main()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);

    auto window = SDL_CreateWindow("Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_ALLOW_HIGHDPI);

    VkApplicationInfo appInfo {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "";
    appInfo.pEngineName = "";
    appInfo.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char *> enabledExtensions {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    };

    std::vector<const char *> enabledLayers {
        "VK_LAYER_LUNARG_standard_validation",
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
        instanceInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
        instanceInfo.ppEnabledExtensionNames = enabledExtensions.data();
    }

    VkInstance instance;
    vkCreateInstance(&instanceInfo, nullptr, &instance);

    Input input;

    auto run = true;
    auto lastTime = 0.0f;

    while (run)
    {
        input.beginUpdate(window);

        SDL_Event evt;
        while (SDL_PollEvent(&evt))
        {
            input.processEvent(evt);

            if (evt.type == SDL_QUIT ||
                evt.type == SDL_WINDOWEVENT && evt.window.event == SDL_WINDOWEVENT_CLOSE ||
                evt.type == SDL_KEYUP && evt.key.keysym.sym == SDLK_ESCAPE)
            {
                run = false;
            }
        }

        auto time = SDL_GetTicks() / 1000.0f;
        auto dt = time - lastTime;
        lastTime = time;
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
