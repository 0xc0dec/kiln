/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "VulkanSwapchain.h"
#include "VulkanDevice.h"

static auto getSwapchainImages(VkDevice device, VkSwapchainKHR swapchain) -> std::vector<VkImage>
{
    uint32_t imageCount = 0;
    KL_VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));
    
    std::vector<VkImage> images;
    images.resize(imageCount);
    KL_VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data()));

    return images;
}

static auto getPresentMode(const vk::Device &device, bool vsync) -> VkPresentModeKHR
{
    uint32_t presentModeCount;
    KL_VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(device.getPhysicalDevice(), device.getSurface(), &presentModeCount, nullptr));

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    KL_VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(device.getPhysicalDevice(), device.getSurface(), &presentModeCount, presentModes.data()));

    auto presentMode = VK_PRESENT_MODE_FIFO_KHR; // "vsync"

    if (!vsync)
    {
        for (const auto mode : presentModes)
        {
            if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                presentMode = mode;
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                presentMode = mode;
                break;
            }
        }
    }

    return presentMode;
}

static auto createSwapchain(const vk::Device &device, uint32_t width, uint32_t height, bool vsync) -> vk::Resource<VkSwapchainKHR>
{
    VkSurfaceCapabilitiesKHR capabilities;
    KL_VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.getPhysicalDevice(), device.getSurface(), &capabilities));

    if (capabilities.currentExtent.width != static_cast<uint32_t>(-1))
    {
        width = capabilities.currentExtent.width;
        height = capabilities.currentExtent.height;
    }

    VkSurfaceTransformFlagsKHR transformFlags;
    if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        transformFlags = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    else
        transformFlags = capabilities.currentTransform;

    auto requestedImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && requestedImageCount > capabilities.maxImageCount)
        requestedImageCount = capabilities.maxImageCount;

    auto presentMode = getPresentMode(device, vsync);

    VkSwapchainCreateInfoKHR swapchainInfo{};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.pNext = nullptr;
    swapchainInfo.surface = device.getSurface();
    swapchainInfo.minImageCount = requestedImageCount;
    swapchainInfo.imageFormat = device.getColorFormat();
    swapchainInfo.imageColorSpace = device.getColorSpace();
    swapchainInfo.imageExtent = {width, height};
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.preTransform = static_cast<VkSurfaceTransformFlagBitsKHR>(transformFlags);
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount = 0;
    swapchainInfo.pQueueFamilyIndices = nullptr;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.oldSwapchain = nullptr;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    vk::Resource<VkSwapchainKHR> swapchain{device, vkDestroySwapchainKHR};
    KL_VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, swapchain.cleanRef()));

    return swapchain;
}

vk::Swapchain::Swapchain(const Device &device, uint32_t width, uint32_t height, bool vsync):
    device(device)
{
    const auto colorFormat = device.getColorFormat();
    const auto depthFormat = device.getDepthFormat();

    swapchain = createSwapchain(device, width, height, vsync);

    renderPass = RenderPass(device, RenderPassConfig()
        .withColorAttachment(colorFormat, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        .withDepthAttachment(depthFormat));
    
    depthStencil = Image(device, width, height, 1, 1, depthFormat,
        0,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

    auto images = getSwapchainImages(device, swapchain);
    
    steps.resize(images.size());
    for (uint32_t i = 0; i < images.size(); i++)
    {
        auto view = createImageView(device, colorFormat, VK_IMAGE_VIEW_TYPE_2D, 1, 1, images[i], VK_IMAGE_ASPECT_COLOR_BIT);
        steps[i].framebuffer = createFrameBuffer(device, view, depthStencil.getView(), renderPass, width, height);
        steps[i].image = images[i];
        steps[i].imageView = std::move(view);
        steps[i].cmdBuffer = createCommandBuffer(device, device.getCommandPool());
    }

    presentCompleteSem = createSemaphore(device);
    renderCompleteSem = createSemaphore(device);
}

auto vk::Swapchain::getNextStep() const -> uint32_t
{
    uint32_t step;
    KL_VK_CHECK_RESULT(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, presentCompleteSem, nullptr, &step));
    return step;
}

void vk::Swapchain::recordCommandBuffers(std::function<void(VkFramebuffer, VkCommandBuffer)> issueCommands)
{
    for (size_t i = 0; i < steps.size(); ++i)
    {
        VkCommandBuffer buf = steps[i].cmdBuffer;
        beginCommandBuffer(buf, false);
        issueCommands(steps[i].framebuffer, buf);
        KL_VK_CHECK_RESULT(vkEndCommandBuffer(buf));
    }
}

void vk::Swapchain::presentNext(VkQueue queue, uint32_t step, uint32_t waitSemaphoreCount, const VkSemaphore *waitSemaphores)
{
    queueSubmit(queue, waitSemaphoreCount, waitSemaphores, 1, &renderCompleteSem, 1, &steps[step].cmdBuffer);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &step;
    presentInfo.pWaitSemaphores = &renderCompleteSem;
    presentInfo.waitSemaphoreCount = 1;
    KL_VK_CHECK_RESULT(vkQueuePresentKHR(queue, &presentInfo));
}
