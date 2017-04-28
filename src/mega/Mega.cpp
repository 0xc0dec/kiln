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
        instanceInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
        instanceInfo.ppEnabledExtensionNames = enabledExtensions.data();
    }

    auto instance = vk::Resource<VkInstance>{vkDestroyInstance};
    KL_VK_CHECK_RESULT(vkCreateInstance(&instanceInfo, nullptr, instance.cleanAndExpose()));

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
