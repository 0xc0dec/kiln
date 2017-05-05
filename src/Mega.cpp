/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Input.h"
#include "Vulkan.h"
#include "VulkanRenderPass.h"
#include "VulkanSwapchain.h"

#include <SDL.h>
#include <SDL_syswm.h>
#ifdef KL_WINDOWS
#   include <windows.h>
#endif
#include <vector>

int main()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);

    const uint32_t CanvasWidth = 800;
    const uint32_t CanvasHeight = 600;

    auto window = SDL_CreateWindow("Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, CanvasWidth, CanvasHeight, SDL_WINDOW_ALLOW_HIGHDPI);

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

    auto surface = vk::Resource<VkSurfaceKHR>{instance, vkDestroySurfaceKHR};
    KL_VK_CHECK_RESULT(vkCreateWin32SurfaceKHR(instance, &surfaceInfo, nullptr, surface.cleanAndExpose()));
#endif

    struct
    {
        VkPhysicalDevice device;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memProperties;
    } physicalDevice;

    physicalDevice.device = getPhysicalDevice(instance);
    vkGetPhysicalDeviceProperties(physicalDevice.device, &physicalDevice.properties);
    vkGetPhysicalDeviceFeatures(physicalDevice.device, &physicalDevice.features);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice.device, &physicalDevice.memProperties);

    auto surfaceFormats = getSurfaceFormats(physicalDevice.device, surface);
    auto colorFormat = std::get<0>(surfaceFormats);
    auto colorSpace = std::get<1>(surfaceFormats);

    auto queueIndex = vk::getQueueIndex(physicalDevice.device, surface);
    auto device = vk::createDevice(physicalDevice.device, queueIndex);

    VkQueue queue;
    vkGetDeviceQueue(device, queueIndex, 0, &queue);

    auto depthFormat = vk::getDepthFormat(physicalDevice.device);
    auto commandPool = vk::createCommandPool(device, queueIndex);
    auto depthStencil = vk::createDepthStencil(device, physicalDevice.memProperties, depthFormat, CanvasWidth, CanvasHeight);
    auto renderPass = vk::RenderPassBuilder(device)
        .withColorAttachment(colorFormat)
        .withDepthAttachment(depthFormat)
        .build();
    renderPass.setClear(true, true, {{0, 0, 0, 1}}, {1, 0});

    auto swapchain = vk::Swapchain(device, physicalDevice.device, surface, renderPass, depthStencil.view,
        CanvasWidth, CanvasHeight, false, colorFormat, colorSpace);

    struct
    {
        vk::Resource<VkSemaphore> presentComplete;
        vk::Resource<VkSemaphore> renderComplete;
    } semaphores;
    semaphores.presentComplete = vk::createSemaphore(device);
    semaphores.renderComplete = vk::createSemaphore(device);

    std::vector<VkCommandBuffer> renderCmdBuffers;
    renderCmdBuffers.resize(swapchain.getStepCount());
    createCommandBuffers(device, commandPool, true, swapchain.getStepCount(), renderCmdBuffers.data());

    for (size_t i = 0; i < renderCmdBuffers.size(); i++)
    {
        auto buf = renderCmdBuffers[i];

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        KL_VK_CHECK_RESULT(vkBeginCommandBuffer(buf, &beginInfo));
        
        renderPass.begin(buf, swapchain.getFramebuffer(i), CanvasWidth, CanvasHeight);

        auto vp = VkViewport{0, 0, CanvasWidth, CanvasHeight, 1, 100};
                
        vkCmdSetViewport(buf, 0, 1, &vp);

        VkRect2D scissor{};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = vp.width;
        scissor.extent.height = vp.height;
        vkCmdSetScissor(buf, 0, 1, &scissor);

        renderPass.end(buf);

        KL_VK_CHECK_RESULT(vkEndCommandBuffer(buf));
    }

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

        auto currentSwapchainStep = swapchain.getNextStep(semaphores.presentComplete);

        VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo{};
	    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pWaitDstStageMask = &submitPipelineStages;
	    submitInfo.waitSemaphoreCount = 1;
	    submitInfo.pWaitSemaphores = &semaphores.presentComplete;
	    submitInfo.signalSemaphoreCount = 1;
	    submitInfo.pSignalSemaphores = &semaphores.renderComplete;
        submitInfo.commandBufferCount = 1;
	    submitInfo.pCommandBuffers = &renderCmdBuffers[currentSwapchainStep];
        KL_VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

        auto swapchainHandle = swapchain.getHandle();
        VkPresentInfoKHR presentInfo{};
	    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	    presentInfo.pNext = nullptr;
	    presentInfo.swapchainCount = 1;
	    presentInfo.pSwapchains = &swapchainHandle;
	    presentInfo.pImageIndices = &currentSwapchainStep;
	    presentInfo.pWaitSemaphores = &semaphores.renderComplete;
	    presentInfo.waitSemaphoreCount = 1;
        KL_VK_CHECK_RESULT(vkQueuePresentKHR(queue, &presentInfo));

        KL_VK_CHECK_RESULT(vkQueueWaitIdle(queue));
    }

    vkFreeCommandBuffers(device, commandPool, renderCmdBuffers.size(), renderCmdBuffers.data());

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
