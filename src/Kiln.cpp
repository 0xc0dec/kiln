// TODO RenderPlan/Job system/whatever for submitting to queue and dependency graph
// TODO Return "jobs" from methods that transfer data (or make two versions - sync (using queueWaitIdle) and "async")
// TODO Refactor swapchain a bit more - avoid external call for getNextStep. Create primary render pass inside swapchain

/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Input.h"
#include "FileSystem.h"
#include "Spectator.h"
#include "Camera.h"
#include "Window.h"
#include "ImageData.h"
#include "vulkan/Vulkan.h"
#include "vulkan/VulkanDevice.h"
#include "vulkan/VulkanRenderPass.h"
#include "vulkan/VulkanSwapchain.h"
#include "vulkan/VulkanDescriptorPool.h"
#include "vulkan/VulkanBuffer.h"
#include "vulkan/VulkanPipeline.h"
#include "vulkan/VulkanDescriptorSetLayoutBuilder.h"
#include "vulkan/VulkanImage.h"
#include "vulkan/VulkanDescriptorSetUpdater.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.inl>
#include <glm/gtc/matrix_transform.inl>
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

int main()
{
    const uint32_t CanvasWidth = 1366;
    const uint32_t CanvasHeight = 768;

    Window window{CanvasWidth, CanvasHeight, "Demo"};
    auto device = vk::Device::create(window.getPlatformHandle());

    auto primaryRenderPass = vk::RenderPass(device, vk::RenderPassConfig()
        .withColorAttachment(device.getColorFormat(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        .withDepthAttachment(device.getDepthFormat()));
    // TODO Take into account that clear values must correspond to attachments,
    // so I guess N attachments means N clear values + 1 for depth
    primaryRenderPass.setClear(true, true, {{0, 1, 0, 1}}, {1, 0});

    auto swapchain = vk::Swapchain(device, primaryRenderPass, CanvasWidth, CanvasHeight, false);

    struct
    {
        vk::DescriptorPool descriptorPool;
        vk::Resource<VkDescriptorSetLayout> globalDescSetLayout;
        VkDescriptorSet globalDescriptorSet;

        struct
        {
            vk::Image colorAttachment;
            vk::Image depthAttachment;
            vk::Resource<VkFramebuffer> frameBuffer;
            vk::RenderPass renderPass;
            vk::Resource<VkSemaphore> semaphore;
            vk::Resource<VkCommandBuffer> commandBuffer;
        } offscreen;

        struct
        {
            vk::Resource<VkDescriptorSetLayout> descSetLayout;
            vk::Pipeline pipeline;
            vk::Image texture;
            vk::Buffer modelMatrixBuffer;
            vk::Buffer vertexBuffer;
            vk::Buffer indexBuffer;
            VkDescriptorSet descriptorSet;
        } box;

        struct
        {
            vk::Resource<VkDescriptorSetLayout> descSetLayout;
            vk::Pipeline pipeline;
            vk::Image texture;
            vk::Buffer modelMatrixBuffer;
            vk::Buffer vertexBuffer;
            VkDescriptorSet descriptorSet;
        } skybox;

        struct
        {
            vk::Resource<VkDescriptorSetLayout> descSetLayout;
            vk::Pipeline pipeline;
            vk::Image texture;
            vk::Buffer modelMatrixBuffer;
            vk::Buffer vertexBuffer;
            VkDescriptorSet descriptorSet;
        } screenQuad;

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
            vk::Buffer modelMatrixBuffer;
            VkDescriptorSet redDescSet;
            VkDescriptorSet greenDescSet;
            VkDescriptorSet blueDescSet;
        } axes;
    } scene;

    {
        scene.offscreen.colorAttachment = vk::Image(device, CanvasWidth / 2, CanvasHeight / 2, 1, 1,
            VK_FORMAT_R8G8B8A8_UNORM,
            0,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_VIEW_TYPE_2D,
            VK_IMAGE_ASPECT_COLOR_BIT);
        scene.offscreen.depthAttachment = vk::Image(device, CanvasWidth / 2, CanvasHeight / 2, 1, 1,
            device.getDepthFormat(),
            0,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_IMAGE_VIEW_TYPE_2D,
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

        scene.offscreen.renderPass = vk::RenderPass(device, vk::RenderPassConfig()
            .withColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            .withDepthAttachment(device.getDepthFormat()));

        scene.offscreen.renderPass.setClear(true, true, {{0, 1, 0, 1}}, {1, 0});

        scene.offscreen.frameBuffer = createFrameBuffer(device, scene.offscreen.colorAttachment.getView(),
            scene.offscreen.depthAttachment.getView(), scene.offscreen.renderPass, CanvasWidth / 2, CanvasHeight / 2);

        scene.offscreen.semaphore = createSemaphore(device);

        scene.offscreen.commandBuffer = createCommandBuffer(device, device.getCommandPool());
    }

    struct
    {
        glm::mat4 projectionMatrix;
        glm::mat4 viewMatrix;
    } viewMatrices;

    Camera cam;
    cam.setPerspective(glm::radians(45.0f), CanvasWidth / (CanvasHeight * 1.0f), 0.01f, 100);
    cam.getTransform().setLocalPosition({10, -5, 10});
    cam.getTransform().lookAt({0, 0, 0}, {0, 1, 0});

    viewMatrices.projectionMatrix = cam.getProjectionMatrix();
    viewMatrices.viewMatrix = glm::mat4();

    auto viewMatricesBuffer = vk::Buffer::createUniformHostVisible(device, sizeof(viewMatrices));
    viewMatricesBuffer.update(&viewMatrices);

    scene.descriptorPool = vk::DescriptorPool(device, 20, vk::DescriptorPoolConfig()
        .forDescriptors(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 20)
        .forDescriptors(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 20));

    scene.globalDescSetLayout = vk::DescriptorSetLayoutBuilder(device)
        .withBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();
    scene.globalDescriptorSet = scene.descriptorPool.allocateSet(scene.globalDescSetLayout);

    vk::DescriptorSetUpdater(device)
        .forUniformBuffer(0, scene.globalDescriptorSet, viewMatricesBuffer, 0, sizeof(viewMatrices))
        .updateSets();

    {
        auto vsSrc = fs::readBytes("../../assets/shaders/Textured.vert.spv");
        auto fsSrc = fs::readBytes("../../assets/shaders/Textured.frag.spv");
        auto vs = createShader(device, vsSrc.data(), vsSrc.size());
        auto fs = createShader(device, fsSrc.data(), fsSrc.size());

        glm::mat4 modelMatrix{};
        scene.box.modelMatrixBuffer = vk::Buffer::createUniformHostVisible(device, sizeof(glm::mat4));
        scene.box.modelMatrixBuffer.update(&modelMatrix);

        scene.box.vertexBuffer = vk::Buffer::createDeviceLocal(device, sizeof(float) * boxVertexData.size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, boxVertexData.data());
        scene.box.indexBuffer = vk::Buffer::createDeviceLocal(device, sizeof(uint32_t) * boxIndexData.size(),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT, boxIndexData.data());

        scene.box.descSetLayout = vk::DescriptorSetLayoutBuilder(device)
            .withBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
            .withBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        scene.box.pipeline = vk::Pipeline(device, scene.offscreen.renderPass, vk::PipelineConfig(vs, fs)
            .withDescriptorSetLayout(scene.globalDescSetLayout)
            .withDescriptorSetLayout(scene.box.descSetLayout)
            .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
            .withCullMode(VK_CULL_MODE_NONE)
            .withTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexBinding(0, sizeof(float) * 5, VK_VERTEX_INPUT_RATE_VERTEX)
            .withVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            .withVertexAttribute(1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3));

        scene.box.descriptorSet = scene.descriptorPool.allocateSet(scene.box.descSetLayout);

        auto textureData = ImageData::load2D("../../assets/textures/Cobblestone.png");
        scene.box.texture = vk::Image::create2D(device, textureData);

        vk::DescriptorSetUpdater(device)
            .forUniformBuffer(0, scene.box.descriptorSet, scene.box.modelMatrixBuffer, 0, sizeof(modelMatrix))
            .forTexture(1, scene.box.descriptorSet, scene.box.texture.getView(), scene.box.texture.getSampler(), scene.box.texture.getLayout())
            .updateSets();
    }

    {
        auto vsSrc = fs::readBytes("../../assets/shaders/ScreenQuad.vert.spv");
        auto fsSrc = fs::readBytes("../../assets/shaders/ScreenQuad.frag.spv");
        auto vs = createShader(device, vsSrc.data(), vsSrc.size());
        auto fs = createShader(device, fsSrc.data(), fsSrc.size());

        scene.screenQuad.vertexBuffer = vk::Buffer::createDeviceLocal(device, sizeof(float) * quadVertexData.size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, quadVertexData.data());

        scene.screenQuad.descSetLayout = vk::DescriptorSetLayoutBuilder(device)
            .withBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        scene.screenQuad.pipeline = vk::Pipeline(device, primaryRenderPass, vk::PipelineConfig(vs, fs)
            .withDepthTest(false, false)
            .withDescriptorSetLayout(scene.screenQuad.descSetLayout)
            .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
            .withCullMode(VK_CULL_MODE_NONE)
            .withTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexBinding(0, sizeof(float) * 5, VK_VERTEX_INPUT_RATE_VERTEX)
            .withVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            .withVertexAttribute(1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3));

        scene.screenQuad.descriptorSet = scene.descriptorPool.allocateSet(scene.screenQuad.descSetLayout);

        auto &colorAttachment = scene.offscreen.colorAttachment;
        vk::DescriptorSetUpdater(device)
            .forTexture(0, scene.screenQuad.descriptorSet, colorAttachment.getView(), colorAttachment.getSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            .updateSets();
    }

    {
        auto vsSrc = fs::readBytes("../../assets/shaders/Skybox.vert.spv");
        auto fsSrc = fs::readBytes("../../assets/shaders/Skybox.frag.spv");
        auto vs = createShader(device, vsSrc.data(), vsSrc.size());
        auto fs = createShader(device, fsSrc.data(), fsSrc.size());

        glm::mat4 modelMatrix{};
        scene.skybox.modelMatrixBuffer = vk::Buffer::createUniformHostVisible(device, sizeof(glm::mat4));
        scene.skybox.modelMatrixBuffer.update(&modelMatrix);

        scene.skybox.vertexBuffer = vk::Buffer::createDeviceLocal(device, sizeof(float) * quadVertexData.size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, quadVertexData.data());

        scene.skybox.descSetLayout = vk::DescriptorSetLayoutBuilder(device)
            .withBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
            .withBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        scene.skybox.pipeline = vk::Pipeline(device, scene.offscreen.renderPass, vk::PipelineConfig(vs, fs)
            .withDepthTest(false, false)
            .withDescriptorSetLayout(scene.globalDescSetLayout)
            .withDescriptorSetLayout(scene.skybox.descSetLayout)
            .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
            .withCullMode(VK_CULL_MODE_NONE)
            .withTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexBinding(0, sizeof(float) * 5, VK_VERTEX_INPUT_RATE_VERTEX)
            .withVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            .withVertexAttribute(1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3));

        scene.skybox.descriptorSet = scene.descriptorPool.allocateSet(scene.skybox.descSetLayout);

        auto data = ImageData::loadCube("../../assets/textures/Cubemap_space.ktx");
        scene.skybox.texture = vk::Image::createCube(device, data);

        vk::DescriptorSetUpdater(device)
            .forUniformBuffer(0, scene.skybox.descriptorSet, scene.skybox.modelMatrixBuffer, 0, sizeof(modelMatrix))
            .forTexture(1, scene.skybox.descriptorSet, scene.skybox.texture.getView(), scene.skybox.texture.getSampler(), scene.skybox.texture.getLayout())
            .updateSets();
    }

    {
        Transform t;
        t.setLocalPosition({3, 0, 3});
        glm::mat4 modelMatrix = t.getWorldMatrix();
        scene.axes.modelMatrixBuffer = vk::Buffer::createUniformHostVisible(device, sizeof(glm::mat4));
        scene.axes.modelMatrixBuffer.update(&modelMatrix);

        glm::vec3 red{1.0f, 0, 0};
        scene.axes.redColorUniformBuffer = vk::Buffer::createUniformHostVisible(device, sizeof(glm::vec3));
        scene.axes.redColorUniformBuffer.update(&red);

        glm::vec3 green{0, 1.0f, 0};
        scene.axes.greenColorUniformBuffer = vk::Buffer::createUniformHostVisible(device, sizeof(glm::vec3));
        scene.axes.greenColorUniformBuffer.update(&green);

        glm::vec3 blue{0, 0, 1.0f};
        scene.axes.blueColorUniformBuffer = vk::Buffer::createUniformHostVisible(device, sizeof(glm::vec3));
        scene.axes.blueColorUniformBuffer.update(&blue);

        auto vsSrc = fs::readBytes("../../assets/shaders/Axis.vert.spv");
        auto fsSrc = fs::readBytes("../../assets/shaders/Axis.frag.spv");
        auto vs = createShader(device, vsSrc.data(), vsSrc.size());
        auto fs = createShader(device, fsSrc.data(), fsSrc.size());

        scene.axes.xAxisVertexBuffer = vk::Buffer::createDeviceLocal(device, sizeof(float) * xAxisVertexData.size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, xAxisVertexData.data());
        scene.axes.yAxisVertexBuffer = vk::Buffer::createDeviceLocal(device, sizeof(float) * yAxisVertexData.size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, yAxisVertexData.data());
        scene.axes.zAxisVertexBuffer = vk::Buffer::createDeviceLocal(device, sizeof(float) * zAxisVertexData.size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, zAxisVertexData.data());

        scene.axes.descSetLayout = vk::DescriptorSetLayoutBuilder(device)
            .withBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
            .withBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
            .build();

        scene.axes.pipeline = vk::Pipeline(device, scene.offscreen.renderPass, vk::PipelineConfig(vs, fs)
            .withDescriptorSetLayout(scene.globalDescSetLayout)
            .withDescriptorSetLayout(scene.axes.descSetLayout)
            .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
            .withCullMode(VK_CULL_MODE_NONE)
            .withTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
            .withVertexBinding(0, sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX)
            .withVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0));

        scene.axes.redDescSet = scene.descriptorPool.allocateSet(scene.axes.descSetLayout);
        scene.axes.greenDescSet = scene.descriptorPool.allocateSet(scene.axes.descSetLayout);
        scene.axes.blueDescSet = scene.descriptorPool.allocateSet(scene.axes.descSetLayout);

        vk::DescriptorSetUpdater(device)
            .forUniformBuffer(0, scene.axes.redDescSet, scene.axes.modelMatrixBuffer, 0, sizeof(modelMatrix))
            .forUniformBuffer(1, scene.axes.redDescSet, scene.axes.redColorUniformBuffer, 0, sizeof(glm::vec3))
            .updateSets();

        vk::DescriptorSetUpdater(device)
            .forUniformBuffer(0, scene.axes.greenDescSet, scene.axes.modelMatrixBuffer, 0, sizeof(modelMatrix))
            .forUniformBuffer(1, scene.axes.greenDescSet, scene.axes.greenColorUniformBuffer, 0, sizeof(glm::vec3))
            .updateSets();

        vk::DescriptorSetUpdater(device)
            .forUniformBuffer(0, scene.axes.blueDescSet, scene.axes.modelMatrixBuffer, 0, sizeof(modelMatrix))
            .forUniformBuffer(1, scene.axes.blueDescSet, scene.axes.blueColorUniformBuffer, 0, sizeof(glm::vec3))
            .updateSets();
    }

    // Record command buffers

    {
        VkCommandBuffer buf = scene.offscreen.commandBuffer;
        vk::beginCommandBuffer(buf, false);

        scene.offscreen.renderPass.begin(buf, scene.offscreen.frameBuffer, CanvasWidth / 2, CanvasHeight / 2);

        auto vp = VkViewport{0, 0, CanvasWidth / 2, CanvasHeight / 2, 0, 1};

        vkCmdSetViewport(buf, 0, 1, &vp);

        VkRect2D scissor{{0, 0}, {vp.width, vp.height}};
        vkCmdSetScissor(buf, 0, 1, &scissor);

        std::vector<VkDeviceSize> vertexBufferOffsets = {0};

        // Skybox 
        {
            std::vector<VkBuffer> vertexBuffers = {scene.skybox.vertexBuffer};
            std::vector<VkDescriptorSet> descSets = {scene.globalDescriptorSet, scene.skybox.descriptorSet};
            vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.skybox.pipeline);
            vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.skybox.pipeline.getLayout(), 0, 2, descSets.data(), 0, nullptr);
            vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), vertexBufferOffsets.data());
            vkCmdDraw(buf, 6, 1, 0, 0);
        }

        // Axes
        {
            vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.axes.pipeline);

            std::vector<VkDescriptorSet> descSets = {scene.globalDescriptorSet, scene.axes.redDescSet};
            
            // TODO bind all at once
            std::vector<VkBuffer> vertexBuffers = {scene.axes.xAxisVertexBuffer};
            vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.axes.pipeline.getLayout(), 0, 2, descSets.data(), 0, nullptr);
            vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), vertexBufferOffsets.data());
            vkCmdDraw(buf, 4, 1, 0, 0);

            vertexBuffers[0] = scene.axes.yAxisVertexBuffer;
            descSets[1] = scene.axes.greenDescSet;
            vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.axes.pipeline.getLayout(), 0, 2, descSets.data(), 0, nullptr);
            vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), vertexBufferOffsets.data());
            vkCmdDraw(buf, 4, 1, 0, 0);

            vertexBuffers[0] = scene.axes.zAxisVertexBuffer;
            descSets[1] = scene.axes.blueDescSet;
            vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.axes.pipeline.getLayout(), 0, 2, descSets.data(), 0, nullptr);
            vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), vertexBufferOffsets.data());
            vkCmdDraw(buf, 4, 1, 0, 0);
        }

        // Box
        {
            std::vector<VkBuffer> vertexBuffers = {scene.box.vertexBuffer};
            std::vector<VkDescriptorSet> descSets = {scene.globalDescriptorSet, scene.box.descriptorSet};
            vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.box.pipeline);
            vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.box.pipeline.getLayout(), 0, 2, descSets.data(), 0, nullptr);
            vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), vertexBufferOffsets.data());
            vkCmdBindIndexBuffer(buf, scene.box.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(buf, boxIndexData.size(), 1, 0, 0, 0);
        }

        scene.offscreen.renderPass.end(buf); 

        KL_VK_CHECK_RESULT(vkEndCommandBuffer(buf));
    }

    swapchain.recordRenderCommands([&](uint32_t i, VkCommandBuffer buf)
    {
        vk::beginCommandBuffer(buf, false);

        primaryRenderPass.begin(buf, swapchain.getFramebuffer(i), CanvasWidth, CanvasHeight);

        auto vp = VkViewport{0, 0, static_cast<float>(CanvasWidth), static_cast<float>(CanvasHeight), 0, 1};

        vkCmdSetViewport(buf, 0, 1, &vp);

        VkRect2D scissor{{0, 0}, {vp.width, vp.height}};
        vkCmdSetScissor(buf, 0, 1, &scissor);

        std::vector<VkBuffer> vertexBuffers = {scene.screenQuad.vertexBuffer};
        std::vector<VkDeviceSize> vertexBufferOffsets = {0};
        std::vector<VkDescriptorSet> descSets = {scene.screenQuad.descriptorSet};
        vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.screenQuad.pipeline);
        vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.screenQuad.pipeline.getLayout(), 0, 1, descSets.data(), 0, nullptr);
        vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), vertexBufferOffsets.data());
        vkCmdDraw(buf, 6, 1, 0, 0);

        primaryRenderPass.end(buf);

        KL_VK_CHECK_RESULT(vkEndCommandBuffer(buf));
    });

    // Main loop

    Input input;

    while (!window.closeRequested())
    {
        window.beginUpdate(input);

        auto dt = window.getTimeDelta();

        applySpectator(cam.getTransform(), input, dt, 1, 5);

        viewMatrices.viewMatrix = cam.getViewMatrix();
        viewMatricesBuffer.update(&viewMatrices);

        auto swapchainStep = swapchain.getNextStep();
        auto presentCompleteSem = swapchain.getPresentCompleteSem();
        vk::queueSubmit(device.getQueue(), 1, &presentCompleteSem, 1, &scene.offscreen.semaphore, 1, &scene.offscreen.commandBuffer);
        swapchain.presentNext(device.getQueue(), swapchainStep, 1, &scene.offscreen.semaphore);
        KL_VK_CHECK_RESULT(vkQueueWaitIdle(device.getQueue()));

        window.endUpdate();
    }

    return 0;
}
