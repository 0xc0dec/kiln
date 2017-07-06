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
        static auto createStaging(VkDevice device, uint32_t size, const vk::PhysicalDevice &physicalDevice,
            const void *initialData = nullptr) -> vk::Buffer;

        Buffer() {}
        Buffer(VkDevice device, const vk::PhysicalDevice &physicalDevice, VkDeviceSize size, VkBufferUsageFlags usageFlags,
            VkMemoryPropertyFlags memPropertyFlags);
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