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
        static auto createStaging(VkDevice device, const PhysicalDevice &physicalDevice, VkDeviceSize size,
            const void *initialData = nullptr) -> Buffer;
        static auto createUniformHostVisible(VkDevice device, const PhysicalDevice &physicalDevice, VkDeviceSize size) -> Buffer;

        Buffer() {}
        Buffer(VkDevice device, const PhysicalDevice &physicalDevice, VkDeviceSize size, VkBufferUsageFlags usageFlags,
            VkMemoryPropertyFlags memPropertyFlags);
        Buffer(Buffer &&other) = default;
        Buffer(const Buffer &other) = delete;
        ~Buffer() {}

        auto operator=(const Buffer &other) -> Buffer& = delete;
        auto operator=(Buffer &&other) -> Buffer& = default;

        operator VkBuffer() { return buffer; }

        auto getHandle() const -> VkBuffer;

        void update(const void *newData) const;
        void transferTo(const Buffer& other, VkQueue queue, VkCommandPool cmdPool) const;

    private:
        VkDevice device = nullptr;
        Resource<VkDeviceMemory> memory;
        Resource<VkBuffer> buffer;
        VkDeviceSize size = 0;
    };

    inline auto Buffer::getHandle() const -> VkBuffer
    {
        return buffer;
    }
}