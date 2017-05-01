/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Common.h"
#include "VulkanResource.h"

#ifdef KL_WINDOWS
#   define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan.h>
#include <functional>

#ifdef KL_DEBUG
#   define KL_VK_CHECK_RESULT(vkCall, ...) KL_PANIC_IF(vkCall != VK_SUCCESS, __VA_ARGS__)
#else
#   define KL_VK_CHECK_RESULT(vkCall) vkCall
#endif

namespace vk
{
    
}
