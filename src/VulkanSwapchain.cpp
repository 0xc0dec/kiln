/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "VulkanSwapchain.h"

vk::Swapchain::Swapchain(Swapchain &&other) noexcept
{
    swap(other);
}

vk::Swapchain::Swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
    VkRenderPass renderPass, VkImageView depthStencilView, uint32_t width, uint32_t height, bool vsync,
    VkFormat colorFormat, VkColorSpaceKHR colorSpace):
    device(device)
{
    VkSurfaceCapabilitiesKHR capabilities;
    KL_VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities));

    uint32_t presentModeCount;
    KL_VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    KL_VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()));

    if (capabilities.currentExtent.width != static_cast<uint32_t>(-1))
    {
        width = capabilities.currentExtent.width;
        height = capabilities.currentExtent.height;
    }

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

    VkSurfaceTransformFlagsKHR transformFlags;
    if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        transformFlags = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    else
        transformFlags = capabilities.currentTransform;

    auto requestedImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && requestedImageCount > capabilities.maxImageCount)
        requestedImageCount = capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR swapchainInfo{};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.pNext = nullptr;
    swapchainInfo.surface = surface;
    swapchainInfo.minImageCount = requestedImageCount;
    swapchainInfo.imageFormat = colorFormat;
    swapchainInfo.imageColorSpace = colorSpace;
    swapchainInfo.imageExtent = {width, height};
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.preTransform = static_cast<VkSurfaceTransformFlagBitsKHR>(transformFlags);
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount = 0;
    swapchainInfo.pQueueFamilyIndices = nullptr;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.oldSwapchain = nullptr; // TODO
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    Resource<VkSwapchainKHR> swapchain{device, vkDestroySwapchainKHR};
    KL_VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, swapchain.cleanAndExpose()));

    uint32_t imageCount = 0;
    KL_VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));
    
    std::vector<VkImage> images;
    images.resize(imageCount);
    KL_VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data()));

    steps.resize(imageCount);

    for (uint32_t i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.pNext = nullptr;
        imageViewInfo.format = colorFormat;
        imageViewInfo.components =
        {
            VK_COMPONENT_SWIZZLE_R,
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A
        };
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.flags = 0;
        imageViewInfo.image = images[i];

        vk::Resource<VkImageView> view{device, vkDestroyImageView};
        KL_VK_CHECK_RESULT(vkCreateImageView(device, &imageViewInfo, nullptr, view.cleanAndExpose()));

        steps[i].framebuffer = createFrameBuffer(device, view, depthStencilView, renderPass, width, height);
        steps[i].image = images[i];
        steps[i].imageView = std::move(view);
    }

    this->swapchain = std::move(swapchain);
}

auto vk::Swapchain::operator=(Swapchain other) noexcept -> Swapchain&
{
    swap(other);
    return *this;
}

auto vk::Swapchain::getNextStep(VkSemaphore semaphore) const -> uint32_t
{
    uint32_t result;
    KL_VK_CHECK_RESULT(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphore, nullptr, &result));
    return result;
}

void vk::Swapchain::swap(Swapchain &other) noexcept
{
    std::swap(device, other.device);
    std::swap(swapchain, other.swapchain);
    std::swap(steps, other.steps);
}