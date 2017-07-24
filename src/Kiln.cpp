// TODO RenderPlan/Job system/whatever for submitting to queue and dependency graph
// TODO Return "jobs" from methods that transfer data (or make two versions - sync (using queueWaitIdle) and "async")

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
#include "MeshData.h"
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

class Offscreen
{
public:
    Offscreen(const vk::Device &device, uint32_t canvasWidth, uint32_t canvasHeight)
    {
        colorAttachment = vk::Image(device, canvasWidth, canvasHeight, 1, 1,
            VK_FORMAT_R8G8B8A8_UNORM,
            0,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_VIEW_TYPE_2D,
            VK_IMAGE_ASPECT_COLOR_BIT);
        depthAttachment = vk::Image(device, canvasWidth, canvasHeight, 1, 1,
            device.getDepthFormat(),
            0,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_IMAGE_VIEW_TYPE_2D,
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

        renderPass = vk::RenderPass(device, vk::RenderPassConfig()
            .withColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, {0, 1, 1, 0})
            .withDepthAttachment(device.getDepthFormat(), true, {1, 0}));

        frameBuffer = createFrameBuffer(device, colorAttachment.getView(),
            depthAttachment.getView(), renderPass, canvasWidth, canvasHeight);

        semaphore = createSemaphore(device);

        commandBuffer = createCommandBuffer(device, device.getCommandPool());
    }

    auto getColorAttachment() -> vk::Image& { return colorAttachment; }
    auto getRenderPass() -> vk::RenderPass& { return renderPass; }
    auto getSemaphore() -> vk::Resource<VkSemaphore>& { return semaphore; }
    auto getCommandBuffer() -> vk::Resource<VkCommandBuffer>& { return commandBuffer; }
    auto getFrameBuffer() -> VkFramebuffer { return frameBuffer; }

private:
    vk::Image colorAttachment;
    vk::Image depthAttachment;
    vk::Resource<VkFramebuffer> frameBuffer;
    vk::RenderPass renderPass;
    vk::Resource<VkSemaphore> semaphore;
    vk::Resource<VkCommandBuffer> commandBuffer;
};

class Mesh
{
public:
    Mesh(const vk::Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalDescSetLayout, VkDescriptorSet globalDescSet,
        vk::DescriptorPool &descPool):
        globalDescSet(globalDescSet)
    {
        auto vsSrc = fs::readBytes("../../assets/shaders/Mesh.vert.spv");
        auto fsSrc = fs::readBytes("../../assets/shaders/Mesh.frag.spv");
        auto vs = createShader(device, vsSrc.data(), vsSrc.size());
        auto fs = createShader(device, fsSrc.data(), fsSrc.size());

        glm::mat4 modelMatrix{};
        modelMatrixBuffer = vk::Buffer::createUniformHostVisible(device, sizeof(glm::mat4));
        modelMatrixBuffer.update(&modelMatrix);

        auto data = MeshData::load("../../assets/meshes/Teapot.obj");

        vertexBuffer = vk::Buffer::createDeviceLocal(device, sizeof(float) * data.getVertexData().size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, data.getVertexData().data());
        indexBuffer = vk::Buffer::createDeviceLocal(device, sizeof(uint32_t) * data.getIndexData().size(),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT, data.getIndexData().data());
        indexCount = data.getIndexData().size();

        descSetLayout = vk::DescriptorSetLayoutBuilder(device)
            .withBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
            .withBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        pipeline = vk::Pipeline(device, renderPass, vk::PipelineConfig(vs, fs)
            .withDescriptorSetLayout(globalDescSetLayout)
            .withDescriptorSetLayout(descSetLayout)
            .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
            .withCullMode(VK_CULL_MODE_NONE)
            .withTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexFormat(data.getFormat()));

        descSet = descPool.allocateSet(descSetLayout);

        auto textureData = ImageData::load2D("../../assets/textures/Cobblestone.png");
        texture = vk::Image::create2D(device, textureData);

        vk::DescriptorSetUpdater(device)
            .forUniformBuffer(0, descSet, modelMatrixBuffer, 0, sizeof(modelMatrix))
            .forTexture(1, descSet, texture.getView(), texture.getSampler(), texture.getLayout())
            .updateSets();
    }

    void render(VkCommandBuffer buf)
    {
        VkBuffer vertexBuffer = this->vertexBuffer;
        VkDeviceSize vertexBufferOffset = 0;
        std::vector<VkDescriptorSet> descSets = {globalDescSet, descSet};
        vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getLayout(), 0, 2, descSets.data(), 0, nullptr);
        vkCmdBindVertexBuffers(buf, 0, 1, &vertexBuffer, &vertexBufferOffset);
        vkCmdBindIndexBuffer(buf, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(buf, indexCount, 1, 0, 0, 0);
    }

private:
    vk::Resource<VkDescriptorSetLayout> descSetLayout;
    vk::Pipeline pipeline;
    vk::Image texture;
    vk::Buffer modelMatrixBuffer;
    vk::Buffer vertexBuffer;
    vk::Buffer indexBuffer;
    uint32_t indexCount;
    VkDescriptorSet descSet;
    VkDescriptorSet globalDescSet;
};

class PostProcessor
{
public:
    PostProcessor(const vk::Device &device, Offscreen &offscreen, vk::DescriptorPool &descPool)
    {
        auto vsSrc = fs::readBytes("../../assets/shaders/PostProcess.vert.spv");
        auto fsSrc = fs::readBytes("../../assets/shaders/PostProcess.frag.spv");
        auto vs = createShader(device, vsSrc.data(), vsSrc.size());
        auto fs = createShader(device, fsSrc.data(), fsSrc.size());

        vertexBuffer = vk::Buffer::createDeviceLocal(device, sizeof(float) * quadVertexData.size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, quadVertexData.data());

        descSetLayout = vk::DescriptorSetLayoutBuilder(device)
            .withBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        pipeline = vk::Pipeline(device, offscreen.getRenderPass(), vk::PipelineConfig(vs, fs)
            .withDepthTest(false, false)
            .withDescriptorSetLayout(descSetLayout)
            .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
            .withCullMode(VK_CULL_MODE_NONE)
            .withTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexFormat(VertexFormat{{3, 2}}));

        descSet = descPool.allocateSet(descSetLayout);

        auto &colorAttachment = offscreen.getColorAttachment();
        vk::DescriptorSetUpdater(device)
            .forTexture(0, descSet, colorAttachment.getView(), colorAttachment.getSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            .updateSets();
    }

    void render(VkCommandBuffer buf)
    {
        std::vector<VkBuffer> vertexBuffers = {vertexBuffer};
        std::vector<VkDeviceSize> vertexBufferOffsets = {0};
        std::vector<VkDescriptorSet> descSets = {descSet};
        vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getLayout(), 0, 1, descSets.data(), 0, nullptr);
        vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), vertexBufferOffsets.data());
        vkCmdDraw(buf, 6, 1, 0, 0);
    }

private:
    vk::Resource<VkDescriptorSetLayout> descSetLayout;
    vk::Pipeline pipeline;
    vk::Image texture;
    vk::Buffer modelMatrixBuffer;
    vk::Buffer vertexBuffer;
    VkDescriptorSet descSet;
};

class Skybox
{
public:
    Skybox(const vk::Device &device, vk::DescriptorPool &descPool, VkDescriptorSetLayout globalDescSetLayout,
        VkDescriptorSet globalDescSet, Offscreen &offscreen):
        globalDescSet(globalDescSet)
    {
        auto vsSrc = fs::readBytes("../../assets/shaders/Skybox.vert.spv");
        auto fsSrc = fs::readBytes("../../assets/shaders/Skybox.frag.spv");
        auto vs = createShader(device, vsSrc.data(), vsSrc.size());
        auto fs = createShader(device, fsSrc.data(), fsSrc.size());

        glm::mat4 modelMatrix{};
        modelMatrixBuffer = vk::Buffer::createUniformHostVisible(device, sizeof(glm::mat4));
        modelMatrixBuffer.update(&modelMatrix);

        vertexBuffer = vk::Buffer::createDeviceLocal(device, sizeof(float) * quadVertexData.size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, quadVertexData.data());

        descSetLayout = vk::DescriptorSetLayoutBuilder(device)
            .withBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
            .withBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        pipeline = vk::Pipeline(device, offscreen.getRenderPass(), vk::PipelineConfig(vs, fs)
            .withDepthTest(false, false)
            .withDescriptorSetLayout(globalDescSetLayout)
            .withDescriptorSetLayout(descSetLayout)
            .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
            .withCullMode(VK_CULL_MODE_NONE)
            .withTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexBinding(0, sizeof(float) * 5, VK_VERTEX_INPUT_RATE_VERTEX)
            .withVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            .withVertexAttribute(1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3));

        descSet = descPool.allocateSet(descSetLayout);

        auto data = ImageData::loadCube("../../assets/textures/Cubemap_space.ktx");
        texture = vk::Image::createCube(device, data);

        vk::DescriptorSetUpdater(device)
            .forUniformBuffer(0, descSet, modelMatrixBuffer, 0, sizeof(modelMatrix))
            .forTexture(1, descSet, texture.getView(), texture.getSampler(), texture.getLayout())
            .updateSets();
    }

    void render(VkCommandBuffer buf)
    {
        std::vector<VkBuffer> vertexBuffers = {vertexBuffer};
        std::vector<VkDeviceSize> vertexBufferOffsets = {0};
        std::vector<VkDescriptorSet> descSets = {globalDescSet, descSet};
        vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getLayout(), 0, 2, descSets.data(), 0, nullptr);
        vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), vertexBufferOffsets.data());
        vkCmdDraw(buf, 6, 1, 0, 0);
    }

private:
    vk::Resource<VkDescriptorSetLayout> descSetLayout;
    vk::Pipeline pipeline;
    vk::Image texture;
    vk::Buffer modelMatrixBuffer;
    vk::Buffer vertexBuffer;
    VkDescriptorSet descSet;
    VkDescriptorSet globalDescSet;
};

int main()
{
    const uint32_t CanvasWidth = 1366;
    const uint32_t CanvasHeight = 768;

    Window window{CanvasWidth, CanvasHeight, "Demo"};
    auto device = vk::Device::create(window.getPlatformHandle());
    auto swapchain = vk::Swapchain(device, CanvasWidth, CanvasHeight, false);

    struct
    {
        vk::DescriptorPool descPool;
        vk::Resource<VkDescriptorSetLayout> globalDescSetLayout;
        VkDescriptorSet globalDescSet;

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

    Offscreen offscreen{device, CanvasWidth, CanvasHeight};

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

    scene.descPool = vk::DescriptorPool(device, 20, vk::DescriptorPoolConfig()
        .forDescriptors(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 20)
        .forDescriptors(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 20));

    scene.globalDescSetLayout = vk::DescriptorSetLayoutBuilder(device)
        .withBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();
    scene.globalDescSet = scene.descPool.allocateSet(scene.globalDescSetLayout);

    vk::DescriptorSetUpdater(device)
        .forUniformBuffer(0, scene.globalDescSet, viewMatricesBuffer, 0, sizeof(viewMatrices))
        .updateSets();

    Mesh mesh{device, offscreen.getRenderPass(), scene.globalDescSetLayout, scene.globalDescSet, scene.descPool};
    PostProcessor postProcessor{device, offscreen, scene.descPool};
    Skybox skybox{device, scene.descPool, scene.globalDescSetLayout, scene.globalDescSet, offscreen};

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

        scene.axes.pipeline = vk::Pipeline(device, offscreen.getRenderPass(), vk::PipelineConfig(vs, fs)
            .withDescriptorSetLayout(scene.globalDescSetLayout)
            .withDescriptorSetLayout(scene.axes.descSetLayout)
            .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
            .withCullMode(VK_CULL_MODE_NONE)
            .withTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
            .withVertexBinding(0, sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX)
            .withVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0));

        scene.axes.redDescSet = scene.descPool.allocateSet(scene.axes.descSetLayout);
        scene.axes.greenDescSet = scene.descPool.allocateSet(scene.axes.descSetLayout);
        scene.axes.blueDescSet = scene.descPool.allocateSet(scene.axes.descSetLayout);

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
        VkCommandBuffer buf = offscreen.getCommandBuffer();
        vk::beginCommandBuffer(buf, false);

        offscreen.getRenderPass().begin(buf, offscreen.getFrameBuffer(), CanvasWidth, CanvasHeight);

        auto vp = VkViewport{0, 0, CanvasWidth, CanvasHeight, 0, 1};

        vkCmdSetViewport(buf, 0, 1, &vp);

        VkRect2D scissor{{0, 0}, {vp.width, vp.height}};
        vkCmdSetScissor(buf, 0, 1, &scissor);

        skybox.render(buf);

        // Axes
        {
            vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.axes.pipeline);

            std::vector<VkDescriptorSet> descSets = {scene.globalDescSet, scene.axes.redDescSet};
            
            std::vector<VkDeviceSize> vertexBufferOffsets = {0};

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

        mesh.render(buf);

        offscreen.getRenderPass().end(buf); 

        KL_VK_CHECK_RESULT(vkEndCommandBuffer(buf));
    }

    swapchain.recordCommandBuffers([&](VkFramebuffer fb, VkCommandBuffer buf)
    {
        swapchain.getRenderPass().begin(buf, fb, CanvasWidth, CanvasHeight);

        auto vp = VkViewport{0, 0, static_cast<float>(CanvasWidth), static_cast<float>(CanvasHeight), 0, 1};

        vkCmdSetViewport(buf, 0, 1, &vp);

        VkRect2D scissor{{0, 0}, {vp.width, vp.height}};
        vkCmdSetScissor(buf, 0, 1, &scissor);

        postProcessor.render(buf);

        swapchain.getRenderPass().end(buf);
    });

    // Main loop

    Input input;

    while (!window.closeRequested() && !input.isKeyPressed(SDLK_ESCAPE, true))
    {
        window.beginUpdate(input);

        auto dt = window.getTimeDelta();

        applySpectator(cam.getTransform(), input, dt, 1, 5);

        viewMatrices.viewMatrix = cam.getViewMatrix();
        viewMatricesBuffer.update(&viewMatrices);

        auto presentCompleteSemaphore = swapchain.acquireNext();
        vk::queueSubmit(device.getQueue(), 1, &presentCompleteSemaphore, 1, &offscreen.getSemaphore(), 1, &offscreen.getCommandBuffer());
        swapchain.presentNext(device.getQueue(), 1, &offscreen.getSemaphore());
        KL_VK_CHECK_RESULT(vkQueueWaitIdle(device.getQueue()));

        window.endUpdate();
    }

    return 0;
}
