/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Vulkan.h"
#include "VulkanSwapchain.h"
#include <vector>
#include <array>

using namespace vk;

auto vk::getPhysicalDevice(VkInstance instance) -> VkPhysicalDevice
{
    uint32_t gpuCount = 0;
    KL_VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));

    std::vector<VkPhysicalDevice> devices(gpuCount);
    KL_VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, devices.data()));

    return devices[0]; // TODO at least for now
}

auto vk::createDevice(VkPhysicalDevice physicalDevice, uint32_t queueIndex) -> Resource<VkDevice>
{
    std::vector<float> queuePriorities = {0.0f};
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = queuePriorities.data();

    std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkDeviceCreateInfo deviceCreateInfo{};
    std::vector<VkPhysicalDeviceFeatures> enabledFeatures{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.pEnabledFeatures = enabledFeatures.data();
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    Resource<VkDevice> result{vkDestroyDevice};
    KL_VK_CHECK_RESULT(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, result.cleanRef()));

    return result;
}

auto vk::getSurfaceFormats(VkPhysicalDevice device, VkSurfaceKHR surface) -> std::tuple<VkFormat, VkColorSpaceKHR>
{
    uint32_t count;
    KL_VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr));

    std::vector<VkSurfaceFormatKHR> formats(count);
    KL_VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats.data()));

    if (count == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
        return std::make_tuple(VK_FORMAT_B8G8R8A8_UNORM, formats[0].colorSpace);
    return std::make_tuple(formats[0].format, formats[0].colorSpace);
}

auto vk::getQueueIndex(VkPhysicalDevice device, VkSurfaceKHR surface) -> uint32_t
{
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

    std::vector<VkQueueFamilyProperties> queueProps;
    queueProps.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, queueProps.data());

    std::vector<VkBool32> presentSupported(count);
    for (uint32_t i = 0; i < count; i++)
    KL_VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupported[i]));

    // TODO support for separate rendering and presenting queues
    for (uint32_t i = 0; i < count; i++)
    {
        if (queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupported[i] == VK_TRUE)
            return i;
    }

    KL_PANIC("Could not find queue index");
    return 0;
}

auto vk::getDepthFormat(VkPhysicalDevice device) -> VkFormat
{
    std::vector<VkFormat> depthFormats =
    {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    for (auto &format : depthFormats)
    {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(device, format, &formatProps);
        // Format must support depth stencil attachment for optimal tiling
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            return format;
    }

    return VK_FORMAT_UNDEFINED;
}

auto vk::createCommandPool(VkDevice device, uint32_t queueIndex) -> Resource<VkCommandPool>
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    Resource<VkCommandPool> commandPool{device, vkDestroyCommandPool};
    KL_VK_CHECK_RESULT(vkCreateCommandPool(device, &poolInfo, nullptr, commandPool.cleanRef()));

    return commandPool;
}

auto vk::createDepthStencil(VkDevice device, VkPhysicalDeviceMemoryProperties physicalDeviceMemProps, VkFormat depthFormat,
    uint32_t canvasWidth, uint32_t canvasHeight) -> DepthStencil
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext = nullptr;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = depthFormat;
    imageInfo.extent = {canvasWidth, canvasHeight, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.flags = 0;

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.allocationSize = 0;
    allocInfo.memoryTypeIndex = 0;

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext = nullptr;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.flags = 0;
    viewInfo.subresourceRange = {};
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkMemoryRequirements memReqs;
    Resource<VkImage> image{device, vkDestroyImage};
    KL_VK_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, image.cleanRef()));
    vkGetImageMemoryRequirements(device, image, &memReqs);

    auto memTypeIndex = findMemoryType(physicalDeviceMemProps, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    KL_PANIC_IF(memTypeIndex < 0);

    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = memTypeIndex;

    Resource<VkDeviceMemory> mem{device, vkFreeMemory};
    KL_VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, mem.cleanRef()));
    KL_VK_CHECK_RESULT(vkBindImageMemory(device, image, mem, 0));

    viewInfo.image = image;
    Resource<VkImageView> view{device, vkDestroyImageView};
    KL_VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, view.cleanRef()));

    DepthStencil result;
    result.image = std::move(image);
    result.mem = std::move(mem);
    result.view = std::move(view);

    return result;
}

auto vk::findMemoryType(VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties, uint32_t typeBits, VkMemoryPropertyFlags properties) -> int32_t
{
    for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1)
        {
            if ((physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }
        typeBits >>= 1;
    }
    return -1;
}

auto vk::createFrameBuffer(VkDevice device, VkImageView colorAttachment, VkImageView depthAttachment, VkRenderPass renderPass,
    uint32_t width, uint32_t height) -> Resource<VkFramebuffer>
{
    std::array<VkImageView, 2> attachments = {colorAttachment, depthAttachment};

    VkFramebufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = attachments.size();
    createInfo.pAttachments = attachments.data();
    createInfo.width = width;
    createInfo.height = height;
    createInfo.layers = 1;

    Resource<VkFramebuffer> frameBuffer{device, vkDestroyFramebuffer};
    KL_VK_CHECK_RESULT(vkCreateFramebuffer(device, &createInfo, nullptr, frameBuffer.cleanRef()));

    return frameBuffer;
}

auto vk::createSemaphore(VkDevice device) -> Resource<VkSemaphore>
{
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    Resource<VkSemaphore> semaphore{device, vkDestroySemaphore};
    KL_VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, semaphore.cleanRef()));

    return semaphore;
}

void vk::createCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t count, VkCommandBuffer *result)
{
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.pNext = nullptr;
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = count;

    KL_VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocateInfo, result));
}

auto vk::createCommandBuffer(VkDevice device, VkCommandPool commandPool) -> VkCommandBuffer
{
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.pNext = nullptr;
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer buffer;
    KL_VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocateInfo, &buffer));

    return buffer;
}

auto vk::createShader(VkDevice device, const void *data, uint32_t size) -> Resource<VkShaderModule>
{
    VkShaderModuleCreateInfo shaderModuleInfo{};
    shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleInfo.pNext = nullptr;
    shaderModuleInfo.flags = 0;
    shaderModuleInfo.codeSize = size;
    shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(data);

    Resource<VkShaderModule> module{device, vkDestroyShaderModule};
    KL_VK_CHECK_RESULT(vkCreateShaderModule(device, &shaderModuleInfo, nullptr, module.cleanRef()));

    return module;
}

auto vk::createShaderStageInfo(bool vertex, VkShaderModule shader, const char *entryPoint) -> VkPipelineShaderStageCreateInfo
{
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.stage = vertex ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
    info.module = shader;
    info.pName = entryPoint;
    info.pSpecializationInfo = nullptr;
    return info;
}

void vk::queueSubmit(VkQueue queue, uint32_t waitSemaphoreCount, const VkSemaphore *waitSemaphores,
    uint32_t signalSemaphoreCount, const VkSemaphore *signalSemaphores,
    uint32_t commandBufferCount, const VkCommandBuffer *commandBuffers)
{
    VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &submitPipelineStages;
    submitInfo.waitSemaphoreCount = waitSemaphoreCount;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.signalSemaphoreCount = signalSemaphoreCount;
    submitInfo.pSignalSemaphores = signalSemaphores;
    submitInfo.commandBufferCount = commandBufferCount;
    submitInfo.pCommandBuffers = commandBuffers;
    KL_VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
}

void vk::queuePresent(VkQueue queue, const vk::Swapchain &swapchain, uint32_t swapchainStep,
    uint32_t waitSemaphoreCount, const VkSemaphore *waitSemaphores)
{
    auto swapchainHandle = swapchain.getHandle();
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchainHandle;
    presentInfo.pImageIndices = &swapchainStep;
    presentInfo.pWaitSemaphores = waitSemaphores;
    presentInfo.waitSemaphoreCount = waitSemaphoreCount;
    KL_VK_CHECK_RESULT(vkQueuePresentKHR(queue, &presentInfo));
}

void vk::setImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
    VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
{
    VkImageMemoryBarrier imageMemoryBarrier{};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (oldImageLayout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            // Image layout is undefined (or does not matter)
            // Only valid as initial layout
            // No flags required, listed only for completeness
            imageMemoryBarrier.srcAccessMask = 0;
            break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            // Image is preinitialized
            // Only valid as initial layout for linear images, preserves memory contents
            // Make sure host writes have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image is a color attachment
            // Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image is a depth/stencil attachment
            // Make sure any writes to the depth/stencil buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image is a transfer source 
            // Make sure any reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image is a transfer destination
            // Make sure any writes to the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image is read by a shader
            // Make sure any shader reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newImageLayout)
    {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image will be used as a transfer destination
            // Make sure any writes to the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image will be used as a transfer source
            // Make sure any reads from the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image will be used as a color attachment
            // Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image layout will be used as a depth/stencil attachment
            // Make sure any writes to depth/stencil buffer have been finished
            imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image will be read in a shader (sampler, input attachment)
            // Make sure any writes to the image have been finished
            if (imageMemoryBarrier.srcAccessMask == 0)
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            break;
    }

    vkCmdPipelineBarrier(cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

auto vk::createDebugCallback(VkInstance instance, PFN_vkDebugReportCallbackEXT callbackFunc) -> VkDebugReportCallbackEXT
{
    VkDebugReportCallbackCreateInfoEXT createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = callbackFunc;

    auto create = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
    KL_PANIC_IF(!create, "Failed to load pointer to vkCreateDebugReportCallbackEXT");

    auto destroy = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));
    KL_PANIC_IF(!destroy, "Failed to load pointer to vkDestroyDebugReportCallbackEXT");

    Resource<VkDebugReportCallbackEXT> result{instance, destroy};
    KL_VK_CHECK_RESULT(create(instance, &createInfo, nullptr, result.cleanRef()));

    return result;
}