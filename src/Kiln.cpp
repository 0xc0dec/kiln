/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Input.h"
#include "FileSystem.h"
#include "Spectator.h"
#include "Camera.h"
#include "Window.h"
#include "vulkan/Vulkan.h"
#include "vulkan/VulkanRenderPass.h"
#include "vulkan/VulkanSwapchain.h"
#include "vulkan/VulkanDescriptorPool.h"
#include "vulkan/VulkanBuffer.h"
#include "vulkan/VulkanPipeline.h"
#include "vulkan/VulkanDescriptorSetLayoutBuilder.h"
#include "vulkan/VulkanTexture.h"
#include "vulkan/VulkanDescriptorSetUpdater.h"

#undef max // gli does not compile otherwise, probably because of Windows.h included earlier
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.inl>
#include <glm/gtc/matrix_transform.inl>
#include <gli/gli.hpp>
#include <vector>

static auto createQuadMeshBuffer(VkDevice device, VkQueue queue, VkCommandPool cmdPool, VkPhysicalDeviceMemoryProperties physDeviceMemProps) -> vk::Buffer
{
    std::vector<float> vertices = {
        /* position */  1,  1, 0,  /* uv */ 1, 0,
        /* position */ -1,  1, 0,  /* uv */ 0, 0,
        /* position */ -1, -1, 0,  /* uv */ 0, 1,
        
        /* position */  1,  1, 0,  /* uv */ 1, 0,
        /* position */ -1, -1, 0,  /* uv */ 0, 1,
        /* position */  1, -1, 0,  /* uv */ 1, 1
    };

    auto vertexBufSize = sizeof(float) * vertices.size();
    auto stagingVertexBuf = vk::Buffer(device, vertexBufSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        physDeviceMemProps);
    stagingVertexBuf.update(vertices.data());

    auto vertexBuf = vk::Buffer(device, vertexBufSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        physDeviceMemProps);
    stagingVertexBuf.transferTo(vertexBuf, queue, cmdPool);

    return std::move(vertexBuf);
}

int main()
{
    const uint32_t canvasWidth = 1366;
    const uint32_t canvasHeight = 768;

    Window window{canvasWidth, canvasHeight, "Demo"};

    vk::PhysicalDevice physicalDevice;

    physicalDevice.device = vk::getPhysicalDevice(window.getInstance());
    vkGetPhysicalDeviceProperties(physicalDevice.device, &physicalDevice.properties);
    vkGetPhysicalDeviceFeatures(physicalDevice.device, &physicalDevice.features);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice.device, &physicalDevice.memoryProperties);

    auto surfaceFormats = vk::getSurfaceFormats(physicalDevice.device, window.getSurface());
    auto colorFormat = std::get<0>(surfaceFormats);
    auto colorSpace = std::get<1>(surfaceFormats);

    auto queueIndex = vk::getQueueIndex(physicalDevice.device, window.getSurface());
    auto device = vk::createDevice(physicalDevice.device, queueIndex);

    VkQueue queue;
    vkGetDeviceQueue(device, queueIndex, 0, &queue);

    auto depthFormat = vk::getDepthFormat(physicalDevice.device);
    auto commandPool = vk::createCommandPool(device, queueIndex);
    auto depthStencil = vk::createDepthStencil(device, physicalDevice.memoryProperties, depthFormat, canvasWidth, canvasHeight);
    auto renderPass = vk::RenderPassBuilder(device)
        .withColorAttachment(colorFormat)
        .withDepthAttachment(depthFormat)
        .build();
    renderPass.setClear(true, true, {{0, 1, 0, 1}}, {1, 0});

    auto swapchain = vk::Swapchain(device, physicalDevice.device, window.getSurface(), renderPass, depthStencil.view,
        canvasWidth, canvasHeight, false, colorFormat, colorSpace);

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

    struct
    {
        glm::mat4 projectionMatrix;
        glm::mat4 modelMatrix;
        glm::mat4 viewMatrix;
    } uniformBuf;

    Camera cam;
    cam.setPerspective(glm::radians(45.0f), canvasWidth / (canvasHeight * 1.0f), 0.01f, 100.0f);
    cam.getTransform().setLocalPosition({0, 0, -5});
    cam.getTransform().lookAt({0, 0, 0}, {0, 1, 0});

    uniformBuf.projectionMatrix = cam.getProjectionMatrix();
    uniformBuf.modelMatrix = glm::mat4();

    auto uniformBuffer = vk::Buffer(device, sizeof(uniformBuf),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        physicalDevice.memoryProperties);
    uniformBuffer.update(&uniformBuf);

    auto vertexBuf = createQuadMeshBuffer(device, queue, commandPool, physicalDevice.memoryProperties);

    struct
    {
        vk::Resource<VkDescriptorSetLayout> descSetLayout;
        vk::DescriptorPool descriptorPool;
        vk::Pipeline pipeline;
        vk::Texture texture;
        VkDescriptorSet descriptorSet;
    } quad;

    {
        auto vsSrc = fs::readBytes("../../assets/Textured.vert.spv");
        auto fsSrc = fs::readBytes("../../assets/Textured.frag.spv");
        auto vs = vk::createShader(device, vsSrc.data(), vsSrc.size());
        auto fs = vk::createShader(device, fsSrc.data(), fsSrc.size());

        quad.descSetLayout = vk::DescriptorSetLayoutBuilder(device)
            .withBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
            .withBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        quad.pipeline = vk::PipelineBuilder(device, renderPass, vs, fs)
            .withDescriptorSetLayouts(&quad.descSetLayout, 1)
            .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
            .withCullMode(VK_CULL_MODE_NONE)
            .withTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexBinding(0, sizeof(float) * 5, VK_VERTEX_INPUT_RATE_VERTEX)
            .withVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            .withVertexAttribute(1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3)
            .withVertexSize(sizeof(float) * 5)
            .build();

        quad.descriptorPool = vk::DescriptorPoolBuilder(device)
            .forDescriptors(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
            .forDescriptors(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
            .build(2);

        quad.descriptorSet = quad.descriptorPool.allocateSet(quad.descSetLayout);

        gli::texture2d textureData(gli::load("../../assets/MetalPlate_rgba.ktx"));
        quad.texture = vk::Texture::create2D(device, physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, textureData, commandPool, queue);

        vk::DescriptorSetUpdater()
            .forUniformBuffer(0, quad.descriptorSet, uniformBuffer.getHandle(), 0, sizeof(uniformBuf))
            .forTexture(1, quad.descriptorSet, quad.texture.getView(), quad.texture.getSampler(), quad.texture.getLayout())
            .updateSets(device);
    }

    struct
    {
        vk::Resource<VkDescriptorSetLayout> descSetLayout;
        vk::DescriptorPool descriptorPool;
        vk::Pipeline pipeline;
        vk::Texture texture;
        VkDescriptorSet descriptorSet;
    } skybox;

    {
        auto skyboxVsSrc = fs::readBytes("../../assets/Skybox.vert.spv");
        auto skyboxFsSrc = fs::readBytes("../../assets/Skybox.frag.spv");
        auto skyboxVs = vk::createShader(device, skyboxVsSrc.data(), skyboxVsSrc.size());
        auto skyboxFs = vk::createShader(device, skyboxFsSrc.data(), skyboxFsSrc.size());

        skybox.descSetLayout = vk::DescriptorSetLayoutBuilder(device)
            .withBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
            .withBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        skybox.pipeline = vk::PipelineBuilder(device, renderPass, skyboxVs, skyboxFs)
            .withDescriptorSetLayouts(&skybox.descSetLayout, 1)
            .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
            .withCullMode(VK_CULL_MODE_NONE)
            .withTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexBinding(0, sizeof(float) * 5, VK_VERTEX_INPUT_RATE_VERTEX)
            .withVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            .withVertexAttribute(1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3)
            .withVertexSize(sizeof(float) * 5)
            .build();

        skybox.descriptorPool = vk::DescriptorPoolBuilder(device)
            .forDescriptors(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
            .forDescriptors(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
            .build(2);

        skybox.descriptorSet = skybox.descriptorPool.allocateSet(skybox.descSetLayout);

        gli::texture_cube textureData(gli::load("../../assets/Cubemap_space.ktx"));
        skybox.texture = vk::Texture::createCube(device, physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, textureData, commandPool, queue);

        vk::DescriptorSetUpdater()
            .forUniformBuffer(0, skybox.descriptorSet, uniformBuffer.getHandle(), 0, sizeof(uniformBuf))
            .forTexture(1, skybox.descriptorSet, skybox.texture.getView(), skybox.texture.getSampler(), skybox.texture.getLayout())
            .updateSets(device);
    }

    for (size_t i = 0; i < renderCmdBuffers.size(); i++)
    {
        auto buf = renderCmdBuffers[i];

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        KL_VK_CHECK_RESULT(vkBeginCommandBuffer(buf, &beginInfo));

        renderPass.begin(buf, swapchain.getFramebuffer(i), canvasWidth, canvasHeight);

        auto vp = VkViewport{0, 0, canvasWidth, canvasHeight, 1, 100};

        vkCmdSetViewport(buf, 0, 1, &vp);

        VkRect2D scissor{};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = vp.width;
        scissor.extent.height = vp.height;
        vkCmdSetScissor(buf, 0, 1, &scissor);

        std::vector<VkBuffer> vertexBuffers = {vertexBuf.getHandle()};
        std::vector<VkDeviceSize> offsets = {0};

        vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, skybox.pipeline);
        vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, skybox.pipeline.getLayout(), 0, 1, &skybox.descriptorSet, 0, nullptr);
        vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), offsets.data());
        vkCmdDraw(buf, 6, 1, 0, 0);

        vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, quad.pipeline);
        vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, quad.pipeline.getLayout(), 0, 1, &quad.descriptorSet, 0, nullptr);
        vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), offsets.data());
        vkCmdDraw(buf, 6, 1, 0, 0);

        renderPass.end(buf);

        KL_VK_CHECK_RESULT(vkEndCommandBuffer(buf));
    }

    while (!window.closeRequested())
    {
        window.beginUpdate();

        auto dt = window.getTimeDelta();

        applySpectator(cam.getTransform(), window.getInput(), dt, 1, 5);

        uniformBuf.viewMatrix = cam.getViewMatrix();
        uniformBuffer.update(&uniformBuf);

        auto currentSwapchainStep = swapchain.getNextStep(semaphores.presentComplete);

        vk::queueSubmit(queue, 1, &semaphores.presentComplete, 1, &semaphores.renderComplete, 1, &renderCmdBuffers[currentSwapchainStep]);
        vk::queuePresent(queue, swapchain, currentSwapchainStep, 1, &semaphores.renderComplete);
        KL_VK_CHECK_RESULT(vkQueueWaitIdle(queue));

        window.endUpdate();
    }

    vkFreeCommandBuffers(device, commandPool, renderCmdBuffers.size(), renderCmdBuffers.data());

    return 0;
}
