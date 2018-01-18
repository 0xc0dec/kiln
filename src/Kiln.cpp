// TODO Refactor ImageData/Font using pointers avoiding pimpl
// TODO Font geometry and rendering
// TODO Use shaderc (see Granite project on Github)

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
#include "Font.h"
#include "Vulkan/Vulkan.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanRenderPass.h"
#include "Vulkan/VulkanSwapchain.h"
#include "Vulkan/VulkanDescriptorPool.h"
#include "Vulkan/VulkanBuffer.h"
#include "Vulkan/VulkanPipeline.h"
#include "Vulkan/VulkanDescriptorSetLayoutBuilder.h"
#include "Vulkan/VulkanImage.h"
#include "Vulkan/VulkanDescriptorSetUpdater.h"
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

class Scene
{
public:
    Scene(const vk::Device &device)
    {
        viewMatricesBuffer = vk::Buffer::createUniformHostVisible(device, sizeof(viewMatrices));

        descPool = vk::DescriptorPool(device, 20, vk::DescriptorPoolConfig()
            .forDescriptors(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 20)
            .forDescriptors(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 20));

        descSetLayout = vk::DescriptorSetLayoutBuilder(device)
            .withBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
            .build();
        descSet = descPool.allocateSet(descSetLayout);

        vk::DescriptorSetUpdater(device)
            .forUniformBuffer(0, descSet, viewMatricesBuffer, 0, sizeof(viewMatrices))
            .updateSets();
    }

    void update(const Camera &cam)
    {
        viewMatrices.proj = cam.getProjectionMatrix();
        viewMatrices.view = cam.getViewMatrix();
        viewMatricesBuffer.update(&viewMatrices);
    }

    auto getDescPool() -> vk::DescriptorPool& { return descPool; }
    auto getDescSetLayout() const -> VkDescriptorSetLayout { return descSetLayout; }
    auto getDescSet() const -> VkDescriptorSet { return descSet; }

private:
    vk::DescriptorPool descPool;
    vk::Resource<VkDescriptorSetLayout> descSetLayout;
    VkDescriptorSet descSet;

    struct
    {
        glm::mat4 proj;
        glm::mat4 view;
    } viewMatrices;

    vk::Buffer viewMatricesBuffer;
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
    auto getFrameBuffer() const -> VkFramebuffer { return frameBuffer; }

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
    Mesh(const vk::Device &device, VkRenderPass renderPass, Scene &scene):
        globalDescSet(scene.getDescSet())
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
            .withDescriptorSetLayout(scene.getDescSetLayout())
            .withDescriptorSetLayout(descSetLayout)
            .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
            .withCullMode(VK_CULL_MODE_NONE)
            .withTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexFormat(data.getFormat()));

        descSet = scene.getDescPool().allocateSet(descSetLayout);

	    const auto textureData = ImageData::load2D("../../assets/textures/Cobblestone.png");
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
    PostProcessor(const vk::Device &device, Offscreen &offscreen, Scene &scene)
    {
        auto vsSrc = fs::readBytes("../../assets/shaders/PostProcess.vert.spv");
        auto fsSrc = fs::readBytes("../../assets/shaders/PostProcess.frag.spv");
	    const auto vs = createShader(device, vsSrc.data(), vsSrc.size());
        const auto fs = createShader(device, fsSrc.data(), fsSrc.size());

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

        descSet = scene.getDescPool().allocateSet(descSetLayout);

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
    Skybox(const vk::Device &device, Offscreen &offscreen, Scene &scene):
        globalDescSet(scene.getDescSet())
    {
        auto vsSrc = fs::readBytes("../../assets/shaders/Skybox.vert.spv");
        auto fsSrc = fs::readBytes("../../assets/shaders/Skybox.frag.spv");
        const auto vs = createShader(device, vsSrc.data(), vsSrc.size());
        const auto fs = createShader(device, fsSrc.data(), fsSrc.size());

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
            .withDescriptorSetLayout(scene.getDescSetLayout())
            .withDescriptorSetLayout(descSetLayout)
            .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
            .withCullMode(VK_CULL_MODE_NONE)
            .withTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexBinding(0, sizeof(float) * 5, VK_VERTEX_INPUT_RATE_VERTEX)
            .withVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            .withVertexAttribute(1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3));

        descSet = scene.getDescPool().allocateSet(descSetLayout);

        const auto data = ImageData::loadCube("../../assets/textures/Cubemap_space.ktx");
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

class Axes
{
public:
    Axes(const vk::Device &device, Offscreen &offscreen, Scene &scene):
        globalDescSet(scene.getDescSet())
    {
        Transform t;
        t.setLocalPosition({3, 0, 3});
	    auto modelMatrix = t.getWorldMatrix();
        modelMatrixBuffer = vk::Buffer::createUniformHostVisible(device, sizeof(glm::mat4));
        modelMatrixBuffer.update(&modelMatrix);

        glm::vec3 red{1.0f, 0, 0};
        redColorUniformBuffer = vk::Buffer::createUniformHostVisible(device, sizeof(glm::vec3));
        redColorUniformBuffer.update(&red);

        glm::vec3 green{0, 1.0f, 0};
        greenColorUniformBuffer = vk::Buffer::createUniformHostVisible(device, sizeof(glm::vec3));
        greenColorUniformBuffer.update(&green);

        glm::vec3 blue{0, 0, 1.0f};
        blueColorUniformBuffer = vk::Buffer::createUniformHostVisible(device, sizeof(glm::vec3));
        blueColorUniformBuffer.update(&blue);

        auto vsSrc = fs::readBytes("../../assets/shaders/Axis.vert.spv");
        auto fsSrc = fs::readBytes("../../assets/shaders/Axis.frag.spv");
        const auto vs = createShader(device, vsSrc.data(), vsSrc.size());
        const auto fs = createShader(device, fsSrc.data(), fsSrc.size());

        xAxisVertexBuffer = vk::Buffer::createDeviceLocal(device, sizeof(float) * xAxisVertexData.size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, xAxisVertexData.data());
        yAxisVertexBuffer = vk::Buffer::createDeviceLocal(device, sizeof(float) * yAxisVertexData.size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, yAxisVertexData.data());
        zAxisVertexBuffer = vk::Buffer::createDeviceLocal(device, sizeof(float) * zAxisVertexData.size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, zAxisVertexData.data());

        descSetLayout = vk::DescriptorSetLayoutBuilder(device)
            .withBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
            .withBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
            .build();

        pipeline = vk::Pipeline(device, offscreen.getRenderPass(), vk::PipelineConfig(vs, fs)
            .withDescriptorSetLayout(scene.getDescSetLayout())
            .withDescriptorSetLayout(descSetLayout)
            .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
            .withCullMode(VK_CULL_MODE_NONE)
            .withTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
            .withVertexBinding(0, sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX)
            .withVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0));

        redDescSet = scene.getDescPool().allocateSet(descSetLayout);
        greenDescSet = scene.getDescPool().allocateSet(descSetLayout);
        blueDescSet = scene.getDescPool().allocateSet(descSetLayout);

        vk::DescriptorSetUpdater(device)
            .forUniformBuffer(0, redDescSet, modelMatrixBuffer, 0, sizeof(modelMatrix))
            .forUniformBuffer(1, redDescSet, redColorUniformBuffer, 0, sizeof(glm::vec3))
            .updateSets();

        vk::DescriptorSetUpdater(device)
            .forUniformBuffer(0, greenDescSet, modelMatrixBuffer, 0, sizeof(modelMatrix))
            .forUniformBuffer(1, greenDescSet, greenColorUniformBuffer, 0, sizeof(glm::vec3))
            .updateSets();

        vk::DescriptorSetUpdater(device)
            .forUniformBuffer(0, blueDescSet, modelMatrixBuffer, 0, sizeof(modelMatrix))
            .forUniformBuffer(1, blueDescSet, blueColorUniformBuffer, 0, sizeof(glm::vec3))
            .updateSets();
    }

    void render(VkCommandBuffer buf)
    {
        vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        std::vector<VkDescriptorSet> descSets = {globalDescSet, redDescSet};
            
        std::vector<VkDeviceSize> vertexBufferOffsets = {0};

        // TODO bind all at once
        std::vector<VkBuffer> vertexBuffers = {xAxisVertexBuffer};
        vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getLayout(), 0, 2, descSets.data(), 0, nullptr);
        vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), vertexBufferOffsets.data());
        vkCmdDraw(buf, 4, 1, 0, 0);

        vertexBuffers[0] = yAxisVertexBuffer;
        descSets[1] = greenDescSet;
        vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getLayout(), 0, 2, descSets.data(), 0, nullptr);
        vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), vertexBufferOffsets.data());
        vkCmdDraw(buf, 4, 1, 0, 0);

        vertexBuffers[0] = zAxisVertexBuffer;
        descSets[1] = blueDescSet;
        vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getLayout(), 0, 2, descSets.data(), 0, nullptr);
        vkCmdBindVertexBuffers(buf, 0, 1, vertexBuffers.data(), vertexBufferOffsets.data());
        vkCmdDraw(buf, 4, 1, 0, 0);
    }

private:
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
    VkDescriptorSet globalDescSet;
};

class Label
{
public:
    Label(const vk::Device &device, const std::string &text, VkRenderPass renderPass, Scene &scene):
        globalDescSet(scene.getDescSet())
    {
        const auto fontData = fs::readBytes("../../assets/Aller.ttf");
        font = Font::createTrueType(device, fontData, 100, 2048, 2048, ' ', '~' - ' ', 2, 2);

        std::vector<float> vertexData;
        std::vector<uint32_t> indexData;

        uint16_t lastIndex = 0;
        float offsetX = 0, offsetY = 0;
        for (auto c : text)
        {
            const auto glyphInfo = font.getGlyphInfo(c, offsetX, offsetY);
            offsetX = glyphInfo.offsetX;
            offsetY = glyphInfo.offsetY;

            vertexData.push_back(glyphInfo.positions[0].x);
            vertexData.push_back(glyphInfo.positions[0].y);
            vertexData.push_back(glyphInfo.positions[0].z);
            vertexData.push_back(glyphInfo.uvs[0].x);
            vertexData.push_back(glyphInfo.uvs[0].y);

            vertexData.push_back(glyphInfo.positions[1].x);
            vertexData.push_back(glyphInfo.positions[1].y);
            vertexData.push_back(glyphInfo.positions[1].z);
            vertexData.push_back(glyphInfo.uvs[1].x);
            vertexData.push_back(glyphInfo.uvs[1].y);

            vertexData.push_back(glyphInfo.positions[2].x);
            vertexData.push_back(glyphInfo.positions[2].y);
            vertexData.push_back(glyphInfo.positions[2].z);
            vertexData.push_back(glyphInfo.uvs[2].x);
            vertexData.push_back(glyphInfo.uvs[2].y);

            vertexData.push_back(glyphInfo.positions[3].x);
            vertexData.push_back(glyphInfo.positions[3].y);
            vertexData.push_back(glyphInfo.positions[3].z);
            vertexData.push_back(glyphInfo.uvs[3].x);
            vertexData.push_back(glyphInfo.uvs[3].y);

            indexData.push_back(lastIndex);
            indexData.push_back(lastIndex + 1);
            indexData.push_back(lastIndex + 2);
            indexData.push_back(lastIndex);
            indexData.push_back(lastIndex + 2);
            indexData.push_back(lastIndex + 3);

            lastIndex += 4;
        }

        auto vsSrc = fs::readBytes("../../assets/shaders/Font.vert.spv");
        auto fsSrc = fs::readBytes("../../assets/shaders/Font.frag.spv");
        const auto vs = createShader(device, vsSrc.data(), vsSrc.size());
        const auto fs = createShader(device, fsSrc.data(), fsSrc.size());

        Transform t;
        t.setLocalScale({0.05f, 0.05f, 0.05f});
        t.setLocalPosition({0, 0, 4});
	    auto modelMatrix = t.getWorldMatrix();
        modelMatrixBuffer = vk::Buffer::createUniformHostVisible(device, sizeof(glm::mat4));
        modelMatrixBuffer.update(&modelMatrix);

        vertexBuffer = vk::Buffer::createDeviceLocal(device, sizeof(float) * vertexData.size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexData.data());
        indexBuffer = vk::Buffer::createDeviceLocal(device, sizeof(uint32_t) * indexData.size(),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexData.data());
        indexCount = indexData.size();

        descSetLayout = vk::DescriptorSetLayoutBuilder(device)
            .withBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS)
            .withBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        const auto vf = VertexFormat({3, 2});

        pipeline = vk::Pipeline(device, renderPass, vk::PipelineConfig(vs, fs)
            .withDescriptorSetLayout(scene.getDescSetLayout())
            .withDescriptorSetLayout(descSetLayout)
            .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
            .withCullMode(VK_CULL_MODE_NONE)
            .withBlend(true, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE,
                VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE)
            .withTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexFormat(vf));

        descSet = scene.getDescPool().allocateSet(descSetLayout);

        vk::DescriptorSetUpdater(device)
            .forUniformBuffer(0, descSet, modelMatrixBuffer, 0, sizeof(modelMatrix))
            .forTexture(1, descSet, font.getAtlas().getView(), font.getAtlas().getSampler(), font.getAtlas().getLayout())
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
    Font font;
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

int main()
{
    const uint32_t canvasWidth = 1366;
    const uint32_t canvasHeight = 768;

    Window window{canvasWidth, canvasHeight, "Demo"};
    auto device = vk::Device::create(window.getPlatformHandle());
    auto swapchain = vk::Swapchain(device, canvasWidth, canvasHeight, false);

    Camera cam;
    cam.setPerspective(glm::radians(45.0f), canvasWidth / (canvasHeight * 1.0f), 0.01f, 100);
    cam.getTransform().setLocalPosition({10, -5, 10});
    cam.getTransform().lookAt({0, 0, 0}, {0, 1, 0});

    Scene scene{device};
    Offscreen offscreen{device, canvasWidth, canvasHeight};
    Mesh mesh{device, offscreen.getRenderPass(), scene};
    PostProcessor postProcessor{device, offscreen, scene};
    Skybox skybox{device, offscreen, scene};
    Axes axes{device, offscreen, scene};
    Label label{device, "Test", offscreen.getRenderPass(), scene};

    // Record command buffers

    {
	    const VkCommandBuffer buf = offscreen.getCommandBuffer();
        vk::beginCommandBuffer(buf, false);

        offscreen.getRenderPass().begin(buf, offscreen.getFrameBuffer(), canvasWidth, canvasHeight);

        auto vp = VkViewport{0, 0, canvasWidth, canvasHeight, 0, 1};

        vkCmdSetViewport(buf, 0, 1, &vp);

        VkRect2D scissor{{0, 0}, {vp.width, vp.height}};
        vkCmdSetScissor(buf, 0, 1, &scissor);

        skybox.render(buf);
        axes.render(buf);
        mesh.render(buf);
        label.render(buf);

        offscreen.getRenderPass().end(buf); 

        KL_VK_CHECK_RESULT(vkEndCommandBuffer(buf));
    }

    swapchain.recordCommandBuffers([&](VkFramebuffer fb, VkCommandBuffer buf)
    {
        swapchain.getRenderPass().begin(buf, fb, canvasWidth, canvasHeight);

        auto vp = VkViewport{0, 0, static_cast<float>(canvasWidth), static_cast<float>(canvasHeight), 0, 1};

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

	    const auto dt = window.getTimeDelta();

        applySpectator(cam.getTransform(), input, dt, 1, 5);
        scene.update(cam);

        auto presentCompleteSemaphore = swapchain.acquireNext();
        vk::queueSubmit(device.getQueue(), 1, &presentCompleteSemaphore, 1, &offscreen.getSemaphore(), 1, &offscreen.getCommandBuffer());
        swapchain.presentNext(device.getQueue(), 1, &offscreen.getSemaphore());
        KL_VK_CHECK_RESULT(vkQueueWaitIdle(device.getQueue()));

        window.endUpdate();
    }

    return 0;
}
