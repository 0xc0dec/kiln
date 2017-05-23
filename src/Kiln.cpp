/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Input.h"
#include "FileSystem.h"
#include "Spectator.h"
#include "Camera.h"
#include "Window.h"
#include "Image.h"
#include "vulkan/Vulkan.h"
#include "vulkan/VulkanRenderPass.h"
#include "vulkan/VulkanSwapchain.h"
#include "vulkan/VulkanDescriptorPool.h"
#include "vulkan/VulkanBuffer.h"
#include "vulkan/VulkanPipeline.h"
#include "vulkan/VulkanDescriptorSetLayoutBuilder.h"

#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.inl>
#include <glm/gtc/matrix_transform.inl>
#include <vector>

static auto createMeshBuffer(VkDevice device, VkQueue queue, VkCommandPool cmdPool, VkPhysicalDeviceMemoryProperties physDeviceMemProps) -> vk::Buffer
{
    std::vector<float> vertices = {
        0.9f, 0.9f, 0,
        -0.9f, 0.9f, 0,
        -0.9f, -0.9f, 0,

        0.9f, 0.8f, 0,
        -0.8f, -0.9f, 0,
        0.9f, -0.9f, 0
    };

    auto vertexBufSize = sizeof(float) * vertices.size();
    auto stagingVertexBuf = vk::Buffer(device, vertexBufSize, vk::Buffer::Host | vk::Buffer::TransferSrc, physDeviceMemProps);
    stagingVertexBuf.update(vertices.data());

    auto vertexBuf = vk::Buffer(device, vertexBufSize, vk::Buffer::Device | vk::Buffer::Vertex | vk::Buffer::TransferDst, physDeviceMemProps);
    stagingVertexBuf.transferTo(vertexBuf, queue, cmdPool);

    return std::move(vertexBuf);
}

int main()
{
    const uint32_t CanvasWidth = 1366;
    const uint32_t CanvasHeight = 768;

    Window window{CanvasWidth, CanvasHeight, "Demo"};

    struct
    {
        VkPhysicalDevice device;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memProperties;
    } physicalDevice;

    physicalDevice.device = vk::getPhysicalDevice(window.getInstance());
    vkGetPhysicalDeviceProperties(physicalDevice.device, &physicalDevice.properties);
    vkGetPhysicalDeviceFeatures(physicalDevice.device, &physicalDevice.features);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice.device, &physicalDevice.memProperties);

    auto surfaceFormats = vk::getSurfaceFormats(physicalDevice.device, window.getSurface());
    auto colorFormat = std::get<0>(surfaceFormats);
    auto colorSpace = std::get<1>(surfaceFormats);

    auto queueIndex = vk::getQueueIndex(physicalDevice.device, window.getSurface());
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

    auto swapchain = vk::Swapchain(device, physicalDevice.device, window.getSurface(), renderPass, depthStencil.view,
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
    vk::createCommandBuffers(device, commandPool, swapchain.getStepCount(), renderCmdBuffers.data());

    // Random test code below

    auto vsBytes = fs::readBytes("../../assets/Test.vert.spv");
    auto fsBytes = fs::readBytes("../../assets/Test.frag.spv");
    auto imageBytes = fs::readBytes("../../assets/Freeman.png");
    auto pngImage = img::loadPNG(imageBytes);

    struct
    {
        vk::Resource<VkDescriptorSetLayout> descSetLayout;
        vk::DescriptorPool descriptorPool;
        vk::Buffer uniformBuffer;
        vk::Pipeline pipeline;
        VkDescriptorSet descriptorSet;
        uint32_t vertexCount;
    } test;

    auto vs = vk::createShader(device, vsBytes.data(), vsBytes.size());
    auto fs = vk::createShader(device, fsBytes.data(), fsBytes.size());

    test.descSetLayout = vk::DescriptorSetLayoutBuilder(device)
        .withBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();

    VkDescriptorSetLayout descSetLayout = test.descSetLayout;
    auto builder = vk::PipelineBuilder(device, renderPass, vs, fs)
        .withDescriptorSetLayouts(&descSetLayout, 1)
        .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
        .withCullMode(VK_CULL_MODE_NONE)
        .withTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    // Three position coordinates
    builder.withVertexBinding(0, sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX);
    builder.withVertexAttribute(0, 0, VK_FORMAT_R32G32_SFLOAT, 0);
    builder.withVertexSize(sizeof(float) * 3);

    test.pipeline = builder.build();
    test.descriptorPool = vk::DescriptorPool(device, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 1);
    test.descriptorSet = test.descriptorPool.allocateSet(test.descSetLayout);

    struct
    {
        glm::mat4 projectionMatrix;
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix;
    } uniformBuf;

    Camera cam;
    cam.getTransform().setLocalPosition({0, 0, -5});
    cam.getTransform().lookAt({0, 0, 0}, {0, 1, 0});

    uniformBuf.projectionMatrix = cam.getProjectionMatrix();
    uniformBuf.modelMatrix = glm::mat4();

    test.uniformBuffer = vk::Buffer(device, sizeof(uniformBuf), vk::Buffer::Uniform | vk::Buffer::Host, physicalDevice.memProperties);
    test.uniformBuffer.update(&uniformBuf);

    VkDescriptorBufferInfo uboInfo{};
    uboInfo.buffer = test.uniformBuffer.getHandle();
    uboInfo.offset = 0;
    uboInfo.range = sizeof(uniformBuf);

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = test.descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &uboInfo;
    descriptorWrite.pImageInfo = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

    auto vertexBuf = createMeshBuffer(device, queue, commandPool, physicalDevice.memProperties);

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

    while (!window.closeRequested())
    {
        window.beginUpdate();

        auto dt = window.getTimeDelta();

        applySpectator(cam.getTransform(), window.getInput(), dt, 1, 1);

        uniformBuf.viewMatrix = cam.getViewMatrix();
        test.uniformBuffer.update(&uniformBuf);

        auto currentSwapchainStep = swapchain.getNextStep(semaphores.presentComplete);

        vk::queueSubmit(queue, 1, &semaphores.presentComplete, 1, &semaphores.renderComplete, 1, &renderCmdBuffers[currentSwapchainStep]);
        vk::queuePresent(queue, swapchain, currentSwapchainStep, 1, &semaphores.renderComplete);
        KL_VK_CHECK_RESULT(vkQueueWaitIdle(queue));

        window.endUpdate();
    }

    vkFreeCommandBuffers(device, commandPool, renderCmdBuffers.size(), renderCmdBuffers.data());

    return 0;
}
