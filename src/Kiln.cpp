/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Input.h"
#include "FileSystem.h"
#include "Vulkan.h"
#include "VulkanRenderPass.h"
#include "VulkanSwapchain.h"
#include "VulkanDescriptorPool.h"
#include "VulkanBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanDescriptorSetLayoutBuilder.h"

#include <SDL.h>
#include <SDL_syswm.h>
#ifdef KL_WINDOWS
#   include <windows.h>
#endif
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
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
    renderPass.setClear(true, true, {{0, 1, 0, 1}}, {1, 0});

    auto swapchain = vk::Swapchain(device, physicalDevice.device, surface, renderPass, depthStencil.view,
        CanvasWidth, CanvasHeight, false, colorFormat, colorSpace);

    struct
    {
        vk::Resource<VkSemaphore> presentComplete;
        vk::Resource<VkSemaphore> renderComplete;
    } semaphores;
    semaphores.presentComplete = vk::createSemaphore(device);
    semaphores.renderComplete = vk::createSemaphore(device);

    struct
    {
        vk::Resource<VkDescriptorSetLayout> descSetLayout;
        vk::DescriptorPool descriptorPool;
        vk::Buffer uniformBuffer;
        vk::Pipeline pipeline;
        VkDescriptorSet descriptorSet;
        uint32_t vertexCount;
    } test;

    auto vsBytes = fs::readBytes("../../assets/Test.vert.spv");
    auto fsBytes = fs::readBytes("../../assets/Test.frag.spv");
    auto vs = vk::createShader(device, vsBytes.data(), vsBytes.size());
    auto fs = vk::createShader(device, fsBytes.data(), fsBytes.size());

    test.descSetLayout = vk::DescriptorSetLayoutBuilder(device)
        .withBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();

    VkDescriptorSetLayout descSetLayoutHandle = test.descSetLayout;
    auto builder = vk::PipelineBuilder(device, renderPass, vs, fs)
        .withDescriptorSetLayouts(&descSetLayoutHandle, 1)
        .withTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    // Two position coordinates
    builder.withVertexBinding(0, sizeof(float) * 2, VK_VERTEX_INPUT_RATE_VERTEX);
    builder.withVertexAttribute(0, 0, VK_FORMAT_R32G32_SFLOAT, 0);
    builder.withVertexSize(sizeof(float) * 2);

    test.pipeline = builder.build();
    test.descriptorPool = vk::DescriptorPool(device, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 1);
    test.descriptorSet = test.descriptorPool.allocateSet(test.descSetLayout);

    auto uniformColor = glm::vec3(0, 0.2f, 0.8f);
    test.uniformBuffer = vk::Buffer(device, sizeof(uniformColor), vk::Buffer::Uniform | vk::Buffer::Host, physicalDevice.memProperties);
    test.uniformBuffer.update(glm::value_ptr(uniformColor));

    VkDescriptorBufferInfo uboInfo{};
    uboInfo.buffer = test.uniformBuffer.getHandle();
    uboInfo.offset = 0;
    uboInfo.range = sizeof(uniformColor);

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = test.descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &uboInfo;
    descriptorWrite.pImageInfo = nullptr; // Optional
    descriptorWrite.pTexelBufferView = nullptr; // Optional

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

    std::vector<VkCommandBuffer> renderCmdBuffers;
    renderCmdBuffers.resize(swapchain.getStepCount());
    createCommandBuffers(device, commandPool, swapchain.getStepCount(), renderCmdBuffers.data());

    std::vector<float> vertices = {
        0.9f, 0.9f,
        -0.9f, 0.9f,
        -0.9f, -0.9f,

        0.9f, 0.8f,
        -0.8f, -0.9f,
        0.9f, -0.9f
    };

    auto bufferSize = sizeof(float) * vertices.size();
    auto stagingVertexBuf = vk::Buffer(device, bufferSize, vk::Buffer::Host | vk::Buffer::TransferSrc, physicalDevice.memProperties);
    stagingVertexBuf.update(vertices.data());

    auto vertexBuf = vk::Buffer(device, bufferSize, vk::Buffer::Device | vk::Buffer::Vertex | vk::Buffer::TransferDst, physicalDevice.memProperties);
    stagingVertexBuf.transferTo(vertexBuf, queue, commandPool);

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

        vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, test.pipeline);
        vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, test.pipeline.getLayout(), 0, 1, &test.descriptorSet, 0, nullptr);

        std::vector<VkBuffer> vertexBuffers = {vertexBuf.getHandle()};
        std::vector<VkDeviceSize> offsets = {0};
        vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), offsets.data());

        vkCmdDraw(buf, 6, 1, 0, 0);

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
