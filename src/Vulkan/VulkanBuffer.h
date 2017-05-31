/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include "Vulkan.h"

namespace vk
{
    class Buffer
    {
    public:
        Buffer() {}
        Buffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memPropertyFlags,
            VkPhysicalDeviceMemoryProperties memProps);
        Buffer(Buffer &&other) noexcept;
        Buffer(const Buffer &other) = delete;

        ~Buffer() {}

        auto operator=(Buffer other) noexcept -> Buffer&;

        auto getHandle() const -> VkBuffer;

        void update(const void *newData) const;
        void transferTo(const Buffer& other, VkQueue queue, VkCommandPool cmdPool) const;

    private:
        VkDevice device = nullptr;
        Resource<VkDeviceMemory> memory;
        Resource<VkBuffer> buffer;
        VkDeviceSize size = 0;

        void swap(Buffer &other) noexcept;
    };

    inline auto Buffer::getHandle() const -> VkBuffer
    {
        return buffer;
    }
}