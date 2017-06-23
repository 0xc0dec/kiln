// TODO Investigate axes directions. Currently Y axis seems pointing down
// TODO Common descriptor sets for view/projection matrices

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

static const std::vector<float> xAxisVertexData = 
{
    0, 0, 0,
    1, 0, 0
};

static const std::vector<float> yAxisVertexData = 
{
    0, 0, 0,
    0, 1, 0
};

static const std::vector<float> zAxisVertexData = 
{
    0, 0, 0,
    0, 0, 1
};

static const std::vector<float> quadVertexData =
{
     1,  1, 0, 1, 0,
    -1,  1, 0, 0, 0,
    -1, -1, 0, 0, 1,
        
     1,  1, 0, 1, 0,
    -1, -1, 0, 0, 1,
     1, -1, 0, 1, 1
};

static const std::vector<float> boxVertexData =
{
    // Negitive X
    -1, -1, -1, 1, 0,
    -1, -1,  1, 0, 0,
    -1,  1,  1, 0, 1,
    -1,  1, -1, 1, 1,

    // Positive X
     1,  1,  1, 1, 1,
     1, -1, -1, 0, 0,
     1,  1, -1, 0, 1,
     1, -1,  1, 1, 0,

    // Positive Y
     1,  1,  1, 1, 1,
     1,  1, -1, 1, 0,
    -1,  1, -1, 0, 0,
    -1,  1,  1, 0, 1,

    // Negative Y
     1, -1,  1, 0, 1,
    -1, -1, -1, 1, 0,
     1, -1, -1, 0, 0,
    -1, -1,  1, 1, 1,

    // Positive Z
    -1,  1,  1, 1, 1,
    -1, -1,  1, 1, 0,
     1, -1,  1, 0, 0,
     1,  1,  1, 0, 1,

    // Negative Z
     1,  1, -1, 1, 1,
    -1, -1, -1, 0, 0,
    -1,  1, -1, 0, 1,
     1, -1, -1, 1, 0
};

static const std::vector<uint32_t> boxIndexData = 
{
    // Negative X
    0, 1, 2,
    0, 2, 3,

    // Positive X
    4, 5, 6,
    5, 4, 7,

    // Positive Y
    8, 9, 10,
    8, 10, 11,
    
    // Negative Y
    12, 13, 14,
    12, 15, 13,

    // Positive Z
    16, 17, 18,
    19, 16, 18,

    // Negative Z
    20, 21, 22,
    20, 23, 21
};

static auto createDeviceLocalBuffer(VkDevice device, VkQueue queue, VkCommandPool cmdPool, const vk::PhysicalDevice &physicalDevice,
    const void *data, uint32_t size, VkBufferUsageFlags usageFlags) -> vk::Buffer
{
    auto stagingBuffer = vk::Buffer(device, size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        physicalDevice.memoryProperties);
    stagingBuffer.update(data);

    auto buffer = vk::Buffer(device, size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageFlags,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        physicalDevice.memoryProperties);
    stagingBuffer.transferTo(buffer, queue, cmdPool);

    return std::move(buffer);
}

int main()
{
    const uint32_t CanvasWidth = 1366;
    const uint32_t CanvasHeight = 768;

    Window window{CanvasWidth, CanvasHeight, "Demo"};

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
    auto depthStencil = vk::createDepthStencil(device, physicalDevice.memoryProperties, depthFormat, CanvasWidth, CanvasHeight);
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

    struct
    {
        vk::DescriptorPool descriptorPool;

        struct
        {
            vk::Resource<VkDescriptorSetLayout> descSetLayout;
            vk::Pipeline pipeline;
            vk::Texture texture;
            VkDescriptorSet descriptorSet;
        } box;

        struct
        {
            vk::Resource<VkDescriptorSetLayout> descSetLayout;
            vk::Pipeline pipeline;
            vk::Texture texture;
            VkDescriptorSet descriptorSet;
        } skybox;

        struct
        {
            vk::Resource<VkDescriptorSetLayout> descSetLayout;
            vk::Pipeline pipeline;
            vk::Buffer redColorUniformBuffer;
            vk::Buffer greenColorUniformBuffer;
            vk::Buffer blueColorUniformBuffer;
            vk::Buffer xAxisVertexBuffer;
            vk::Buffer yAxisVertexBuffer;
            vk::Buffer zAxisVertexBuffer;
            VkDescriptorSet redDescSet;
            VkDescriptorSet greenDescSet;
            VkDescriptorSet blueDescSet;
        } axes;
    } scene;

    struct
    {
        glm::mat4 projectionMatrix;
        glm::mat4 modelMatrix;
        glm::mat4 viewMatrix;
    } matrices;

    Camera cam;
    cam.setPerspective(glm::radians(45.0f), CanvasWidth / (CanvasHeight * 1.0f), 0.01f, 100);
    cam.getTransform().setLocalPosition({10, -5, 10});
    cam.getTransform().lookAt({0, 0, 0}, {0, 1, 0});

    matrices.projectionMatrix = cam.getProjectionMatrix();
    matrices.modelMatrix = glm::mat4();

    auto matrixUniformBuffer = vk::Buffer(device, sizeof(matrices),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        physicalDevice.memoryProperties);
    matrixUniformBuffer.update(&matrices);

    // TODO move to corresponding structs
    auto quadVertexBuffer = createDeviceLocalBuffer(device, queue, commandPool, physicalDevice, quadVertexData.data(),
        sizeof(float) * quadVertexData.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    auto boxVertexBuffer = createDeviceLocalBuffer(device, queue, commandPool, physicalDevice, boxVertexData.data(),
        sizeof(float) * boxVertexData.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    auto boxIndexBuffer = createDeviceLocalBuffer(device, queue, commandPool, physicalDevice,
        boxIndexData.data(), sizeof(uint32_t) * boxIndexData.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    scene.descriptorPool = vk::DescriptorPoolBuilder(device)
        .forDescriptors(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10)
        .forDescriptors(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10)
        .build(10);

    {
        auto vsSrc = fs::readBytes("../../assets/Textured.vert.spv");
        auto fsSrc = fs::readBytes("../../assets/Textured.frag.spv");
        auto vs = vk::createShader(device, vsSrc.data(), vsSrc.size());
        auto fs = vk::createShader(device, fsSrc.data(), fsSrc.size());

        scene.box.descSetLayout = vk::DescriptorSetLayoutBuilder(device)
            .withBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
            .withBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        scene.box.pipeline = vk::PipelineBuilder(device, renderPass, vs, fs)
            .withDescriptorSetLayouts(&scene.box.descSetLayout, 1)
            .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
            .withCullMode(VK_CULL_MODE_BACK_BIT)
            .withTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexBinding(0, sizeof(float) * 5, VK_VERTEX_INPUT_RATE_VERTEX)
            .withVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            .withVertexAttribute(1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3)
            .withVertexSize(sizeof(float) * 5)
            .build();

        scene.box.descriptorSet = scene.descriptorPool.allocateSet(scene.box.descSetLayout);

        gli::texture2d textureData(gli::load("../../assets/MetalPlate_rgba.ktx"));
        scene.box.texture = vk::Texture::create2D(device, physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, textureData, commandPool, queue);

        vk::DescriptorSetUpdater(device)
            .forUniformBuffer(0, scene.box.descriptorSet, matrixUniformBuffer.getHandle(), 0, sizeof(matrices))
            .forTexture(1, scene.box.descriptorSet, scene.box.texture.getView(), scene.box.texture.getSampler(), scene.box.texture.getLayout())
            .updateSets();
    }

    {
        auto vsSrc = fs::readBytes("../../assets/Skybox.vert.spv");
        auto fsSrc = fs::readBytes("../../assets/Skybox.frag.spv");
        auto vs = vk::createShader(device, vsSrc.data(), vsSrc.size());
        auto fs = vk::createShader(device, fsSrc.data(), fsSrc.size());

        scene.skybox.descSetLayout = vk::DescriptorSetLayoutBuilder(device)
            .withBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
            .withBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        scene.skybox.pipeline = vk::PipelineBuilder(device, renderPass, vs, fs)
            .withDescriptorSetLayouts(&scene.skybox.descSetLayout, 1)
            .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
            .withCullMode(VK_CULL_MODE_NONE)
            .withTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexBinding(0, sizeof(float) * 5, VK_VERTEX_INPUT_RATE_VERTEX)
            .withVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            .withVertexAttribute(1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3)
            .withVertexSize(sizeof(float) * 5)
            .build();

        scene.skybox.descriptorSet = scene.descriptorPool.allocateSet(scene.skybox.descSetLayout);

        gli::texture_cube textureData(gli::load("../../assets/Cubemap_space.ktx"));
        scene.skybox.texture = vk::Texture::createCube(device, physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, textureData, commandPool, queue);

        vk::DescriptorSetUpdater(device)
            .forUniformBuffer(0, scene.skybox.descriptorSet, matrixUniformBuffer.getHandle(), 0, sizeof(matrices))
            .forTexture(1, scene.skybox.descriptorSet, scene.skybox.texture.getView(), scene.skybox.texture.getSampler(), scene.skybox.texture.getLayout())
            .updateSets();
    }

    {
        glm::vec3 red{1.0f, 0, 0};
        scene.axes.redColorUniformBuffer = vk::Buffer(device, sizeof(glm::vec3),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            physicalDevice.memoryProperties);
        scene.axes.redColorUniformBuffer.update(&red);

        glm::vec3 green{0, 1.0f, 0};
        scene.axes.greenColorUniformBuffer = vk::Buffer(device, sizeof(glm::vec3),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            physicalDevice.memoryProperties);
        scene.axes.greenColorUniformBuffer.update(&green);

        glm::vec3 blue{0, 0, 1.0f};
        scene.axes.blueColorUniformBuffer = vk::Buffer(device, sizeof(glm::vec3),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            physicalDevice.memoryProperties);
        scene.axes.blueColorUniformBuffer.update(&blue);

        auto vsSrc = fs::readBytes("../../assets/Axis.vert.spv");
        auto fsSrc = fs::readBytes("../../assets/Axis.frag.spv");
        auto vs = vk::createShader(device, vsSrc.data(), vsSrc.size());
        auto fs = vk::createShader(device, fsSrc.data(), fsSrc.size());

        scene.axes.xAxisVertexBuffer = createDeviceLocalBuffer(device, queue, commandPool, physicalDevice, xAxisVertexData.data(),
            sizeof(float) * xAxisVertexData.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        scene.axes.yAxisVertexBuffer = createDeviceLocalBuffer(device, queue, commandPool, physicalDevice, yAxisVertexData.data(),
            sizeof(float) * yAxisVertexData.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        scene.axes.zAxisVertexBuffer = createDeviceLocalBuffer(device, queue, commandPool, physicalDevice, zAxisVertexData.data(),
            sizeof(float) * zAxisVertexData.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

        scene.axes.descSetLayout = vk::DescriptorSetLayoutBuilder(device)
            .withBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
            .withBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
            .build();

        scene.axes.pipeline = vk::PipelineBuilder(device, renderPass, vs, fs)
            .withDescriptorSetLayouts(&scene.axes.descSetLayout, 1)
            .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
            .withCullMode(VK_CULL_MODE_NONE)
            .withTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
            .withVertexBinding(0, sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX)
            .withVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            .withVertexSize(sizeof(float) * 3)
            .build();

        scene.axes.redDescSet = scene.descriptorPool.allocateSet(scene.axes.descSetLayout);
        scene.axes.greenDescSet = scene.descriptorPool.allocateSet(scene.axes.descSetLayout);
        scene.axes.blueDescSet = scene.descriptorPool.allocateSet(scene.axes.descSetLayout);

        vk::DescriptorSetUpdater(device)
            .forUniformBuffer(0, scene.axes.redDescSet, matrixUniformBuffer.getHandle(), 0, sizeof(matrices))
            .forUniformBuffer(1, scene.axes.redDescSet, scene.axes.redColorUniformBuffer.getHandle(), 0, sizeof(glm::vec3))
            .updateSets();

        vk::DescriptorSetUpdater(device)
            .forUniformBuffer(0, scene.axes.greenDescSet, matrixUniformBuffer.getHandle(), 0, sizeof(matrices))
            .forUniformBuffer(1, scene.axes.greenDescSet, scene.axes.greenColorUniformBuffer.getHandle(), 0, sizeof(glm::vec3))
            .updateSets();

        vk::DescriptorSetUpdater(device)
            .forUniformBuffer(0, scene.axes.blueDescSet, matrixUniformBuffer.getHandle(), 0, sizeof(matrices))
            .forUniformBuffer(1, scene.axes.blueDescSet, scene.axes.blueColorUniformBuffer.getHandle(), 0, sizeof(glm::vec3))
            .updateSets();
    }

    for (size_t i = 0; i < renderCmdBuffers.size(); i++)
    {
        auto buf = renderCmdBuffers[i];
        vk::beginCommandBuffer(buf, false);

        renderPass.begin(buf, swapchain.getFramebuffer(i), CanvasWidth, CanvasHeight);

        auto vp = VkViewport{0, 0, CanvasWidth, CanvasHeight, 1, 100};

        vkCmdSetViewport(buf, 0, 1, &vp);

        VkRect2D scissor{{0, 0}, {vp.width, vp.height}};
        vkCmdSetScissor(buf, 0, 1, &scissor);

        std::vector<VkDeviceSize> vertexBufferOffsets = {0};

        // Skybox 
        {
            std::vector<VkBuffer> vertexBuffers = {quadVertexBuffer.getHandle()};
            vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.skybox.pipeline);
            vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.skybox.pipeline.getLayout(), 0, 1, &scene.skybox.descriptorSet, 0, nullptr);
            vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), vertexBufferOffsets.data());
            vkCmdDraw(buf, 6, 1, 0, 0);
        }

        // Axes
        {
            vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.axes.pipeline);
            
            // TODO bind all at once
            std::vector<VkBuffer> vertexBuffers = {scene.axes.xAxisVertexBuffer.getHandle()};
            vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.axes.pipeline.getLayout(), 0, 1, &scene.axes.redDescSet, 0, nullptr);
            vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), vertexBufferOffsets.data());
            vkCmdDraw(buf, 4, 1, 0, 0);

            vertexBuffers[0] = scene.axes.yAxisVertexBuffer.getHandle();
            vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.axes.pipeline.getLayout(), 0, 1, &scene.axes.greenDescSet, 0, nullptr);
            vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), vertexBufferOffsets.data());
            vkCmdDraw(buf, 4, 1, 0, 0);

            vertexBuffers[0] = scene.axes.zAxisVertexBuffer.getHandle();
            vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.axes.pipeline.getLayout(), 0, 1, &scene.axes.blueDescSet, 0, nullptr);
            vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), vertexBufferOffsets.data());
            vkCmdDraw(buf, 4, 1, 0, 0);
        }

        // Box
        {
            std::vector<VkBuffer> vertexBuffers = {boxVertexBuffer.getHandle()};
            vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.box.pipeline);
            vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.box.pipeline.getLayout(), 0, 1, &scene.box.descriptorSet, 0, nullptr);
            vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), vertexBufferOffsets.data());
            vkCmdBindIndexBuffer(buf, boxIndexBuffer.getHandle(), 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(buf, boxIndexData.size(), 1, 0, 0, 0);
        }

        renderPass.end(buf);

        KL_VK_CHECK_RESULT(vkEndCommandBuffer(buf));
    }

    while (!window.closeRequested())
    {
        window.beginUpdate();

        auto dt = window.getTimeDelta();

        applySpectator(cam.getTransform(), window.getInput(), dt, 1, 5);

        matrices.viewMatrix = cam.getViewMatrix();
        matrixUniformBuffer.update(&matrices);

        auto swapchainStep = swapchain.getNextStep(semaphores.presentComplete);

        vk::queueSubmit(queue, 1, &semaphores.presentComplete, 1, &semaphores.renderComplete, 1, &renderCmdBuffers[swapchainStep]);
        vk::queuePresent(queue, swapchain, swapchainStep, 1, &semaphores.renderComplete);
        KL_VK_CHECK_RESULT(vkQueueWaitIdle(queue));

        window.endUpdate();
    }

    vkFreeCommandBuffers(device, commandPool, renderCmdBuffers.size(), renderCmdBuffers.data());

    return 0;
}
