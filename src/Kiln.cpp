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

#undef max // gli does not compile otherwise, probably because of Windows.h included earlier
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.inl>
#include <glm/gtc/matrix_transform.inl>
#include <gli/gli.hpp>
#include <vector>

static auto createMeshBuffer(VkDevice device, VkQueue queue, VkCommandPool cmdPool, VkPhysicalDeviceMemoryProperties physDeviceMemProps) -> vk::Buffer
{
    std::vector<float> vertices = {
        0.9f, 0.9f, 0, 1, 0,
        -0.9f, 0.9f, 0, 0, 0,
        -0.9f, -0.9f, 0, 0, 1,

        0.9f, 0.8f, 0, 1, 0,
        -0.8f, -0.9f, 0, 0, 1,
        0.9f, -0.9f, 0, 1, 1
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
        .withBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    VkDescriptorSetLayout descSetLayout = test.descSetLayout;
    auto builder = vk::PipelineBuilder(device, renderPass, vs, fs)
        .withDescriptorSetLayouts(&descSetLayout, 1)
        .withFrontFace(VK_FRONT_FACE_CLOCKWISE)
        .withCullMode(VK_CULL_MODE_NONE)
        .withTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    // Three position coordinates
    builder.withVertexBinding(0, sizeof(float) * 5, VK_VERTEX_INPUT_RATE_VERTEX);
    builder.withVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
    builder.withVertexAttribute(1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3);
    builder.withVertexSize(sizeof(float) * 5);

    test.pipeline = builder.build();
    test.descriptorPool = vk::DescriptorPool(device,
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
        {1, 1},
        2);
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

    auto vertexBuf = createMeshBuffer(device, queue, commandPool, physicalDevice.memProperties);

    // Texture

    struct
    {
        VkSampler sampler;
        VkImage image;
        VkImageLayout imageLayout;
        VkDeviceMemory deviceMemory;
        VkImageView view;
        uint32_t mipLevels;
        uint32_t width;
        uint32_t height;
    } texture;

    gli::texture2d texData(gli::load("../../assets/metalplate01_rgba.ktx"));
    assert(!texData.empty());
    texture.width = texData[0].extent().x;
    texture.height = texData[0].extent().y;
    texture.mipLevels = texData.levels();

    auto imageStagingBuf = vk::Buffer(device, texData.size(), vk::Buffer::Host | vk::Buffer::TransferSrc, physicalDevice.memProperties);
    imageStagingBuf.update(texData.data());

    std::vector<VkBufferImageCopy> bufferCopyRegions;
    uint32_t offset = 0;

    for (uint32_t i = 0; i < texture.mipLevels; i++)
    {
        VkBufferImageCopy bufferCopyRegion{};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = i;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(texData[i].extent().x);
        bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(texData[i].extent().y);
        bufferCopyRegion.imageExtent.depth = 1;
        bufferCopyRegion.bufferOffset = offset;

        bufferCopyRegions.push_back(bufferCopyRegion);

        offset += static_cast<uint32_t>(texData[i].size());
    }

    auto format = VK_FORMAT_R8G8B8A8_UNORM;

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.mipLevels = texture.mipLevels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // Set initial layout of the image to undefined
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = {texture.width, texture.height, 1};
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    KL_VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &texture.image));

    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(device, texture.image, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = vk::findMemoryType(physicalDevice.memProperties, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    KL_VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &texture.deviceMemory));
    KL_VK_CHECK_RESULT(vkBindImageMemory(device, texture.image, texture.deviceMemory, 0));

    // Image barrier for optimal image

    // The sub resource range describes the regions of the image we will be transition
    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = texture.mipLevels;
    subresourceRange.layerCount = 1;

    auto copyCmdBuf = vk::createCommandBuffer(device, commandPool);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(copyCmdBuf, &beginInfo);

    vk::setImageLayout(
        copyCmdBuf,
        texture.image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        subresourceRange,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    vkCmdCopyBufferToImage(
        copyCmdBuf,
        imageStagingBuf.getHandle(),
        texture.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(bufferCopyRegions.size()),
        bufferCopyRegions.data());

    // Change texture image layout to shader read after all mip levels have been copied
    texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vk::setImageLayout(
		copyCmdBuf,
		texture.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		texture.imageLayout,
		subresourceRange,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    vkEndCommandBuffer(copyCmdBuf);

    vk::queueSubmit(queue, 0, nullptr, 0, nullptr, 1, &copyCmdBuf);
    KL_VK_CHECK_RESULT(vkQueueWaitIdle(queue));

    // Sampler

    VkSamplerCreateInfo sampler{};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.maxAnisotropy = 1.0f;
	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	// Set max level-of-detail to mip level count of the texture
	sampler.maxLod = static_cast<float>(texture.mipLevels);
	// Enable anisotropic filtering
	// This feature is optional, so we must check if it's supported on the device
	if (physicalDevice.features.samplerAnisotropy)
	{
		sampler.maxAnisotropy = physicalDevice.properties.limits.maxSamplerAnisotropy;
		sampler.anisotropyEnable = VK_TRUE;
	}
	else
	{
		sampler.maxAnisotropy = 1.0;
		sampler.anisotropyEnable = VK_FALSE;
	}
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	KL_VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &texture.sampler));

    // Texture view

	VkImageViewCreateInfo view{};
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view.format = format;
	view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view.subresourceRange.baseMipLevel = 0;
	view.subresourceRange.baseArrayLayer = 0;
	view.subresourceRange.layerCount = 1;
	view.subresourceRange.levelCount = texture.mipLevels;
	view.image = texture.image;
	KL_VK_CHECK_RESULT(vkCreateImageView(device, &view, nullptr, &texture.view));

    // Descriptor sets

    std::vector<VkWriteDescriptorSet> writeDescriptorSets;

    VkDescriptorBufferInfo uboInfo{};
    uboInfo.buffer = test.uniformBuffer.getHandle();
    uboInfo.offset = 0;
    uboInfo.range = sizeof(uniformBuf);

    VkWriteDescriptorSet uboDescriptorWrite{};
    uboDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    uboDescriptorWrite.dstSet = test.descriptorSet;
    uboDescriptorWrite.dstBinding = 0;
    uboDescriptorWrite.dstArrayElement = 0;
    uboDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboDescriptorWrite.descriptorCount = 1;
    uboDescriptorWrite.pBufferInfo = &uboInfo;
    uboDescriptorWrite.pImageInfo = nullptr;
    uboDescriptorWrite.pTexelBufferView = nullptr;
    writeDescriptorSets.push_back(uboDescriptorWrite);

    VkDescriptorImageInfo textureDescriptor{};
	textureDescriptor.imageView = texture.view;
	textureDescriptor.sampler = texture.sampler;
	textureDescriptor.imageLayout = texture.imageLayout;

    VkWriteDescriptorSet textureDescriptorWrite{};
    textureDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    textureDescriptorWrite.dstSet = test.descriptorSet;
    textureDescriptorWrite.dstBinding = 1;
    textureDescriptorWrite.dstArrayElement = 0;
    textureDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureDescriptorWrite.descriptorCount = 1;
    textureDescriptorWrite.pBufferInfo = nullptr;
    textureDescriptorWrite.pImageInfo = &textureDescriptor;
    textureDescriptorWrite.pTexelBufferView = nullptr;
    writeDescriptorSets.push_back(textureDescriptorWrite);
    
    vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);

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

        applySpectator(cam.getTransform(), window.getInput(), dt, 1, 5);

        uniformBuf.viewMatrix = cam.getViewMatrix();
        test.uniformBuffer.update(&uniformBuf);

        auto currentSwapchainStep = swapchain.getNextStep(semaphores.presentComplete);

        vk::queueSubmit(queue, 1, &semaphores.presentComplete, 1, &semaphores.renderComplete, 1, &renderCmdBuffers[currentSwapchainStep]);
        vk::queuePresent(queue, swapchain, currentSwapchainStep, 1, &semaphores.renderComplete);
        KL_VK_CHECK_RESULT(vkQueueWaitIdle(queue));

        window.endUpdate();
    }

    vkDestroyImageView(device, texture.view, nullptr);
	vkDestroyImage(device, texture.image, nullptr);
	vkDestroySampler(device, texture.sampler, nullptr);
	vkFreeMemory(device, texture.deviceMemory, nullptr);

    vkFreeCommandBuffers(device, commandPool, renderCmdBuffers.size(), renderCmdBuffers.data());

    return 0;
}
