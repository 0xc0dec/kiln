/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "VulkanBuffer.h"
#include "VulkanDevice.h"

auto vk::Buffer::createStaging(const Device &device, VkDeviceSize size, const void *initialData) -> Buffer
{
    auto buffer = Buffer(device, size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (initialData)
        buffer.update(initialData);

    return buffer;
}

auto vk::Buffer::createUniformHostVisible(const Device &device, VkDeviceSize size) -> Buffer
{
    return Buffer(device, size,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

auto vk::Buffer::createDeviceLocal(const Device &device, VkDeviceSize size, VkBufferUsageFlags usageFlags, const void *data) -> Buffer
{
    auto stagingBuffer = createStaging(device, size, data);

    auto buffer = Buffer(device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    stagingBuffer.transferTo(buffer, device.getQueue(), device.getCommandPool());

    return std::move(buffer);
}

vk::Buffer::Buffer(const Device &device, VkDeviceSize size, VkBufferUsageFlags usageFlags,
    VkMemoryPropertyFlags memPropertyFlags):
    device(device),
    size(size)
{
    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.flags = 0;
    bufferInfo.queueFamilyIndexCount = 0;
    bufferInfo.pQueueFamilyIndices = nullptr;

    buffer = Resource<VkBuffer>{device, vkDestroyBuffer};
    KL_VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, buffer.cleanRef()));

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(device.getPhysicalMemoryFeatures(), memReqs.memoryTypeBits, memPropertyFlags);

    memory = Resource<VkDeviceMemory>{device, vkFreeMemory};
    KL_VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, memory.cleanRef()));
    KL_VK_CHECK_RESULT(vkBindBufferMemory(device, buffer, memory, 0));
}

void vk::Buffer::update(const void *newData) const
{
    void *ptr = nullptr;
	KL_VK_CHECK_RESULT(vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &ptr));
	memcpy(ptr, newData, size);
	vkUnmapMemory(device, memory);
}

void vk::Buffer::transferTo(const Buffer &dst, VkQueue queue, VkCommandPool cmdPool) const
{
    auto cmdBuf = createCommandBuffer(device, cmdPool);
    beginCommandBuffer(cmdBuf, true);

    VkBufferCopy copyRegion{};
    copyRegion.size = dst.size;
    vkCmdCopyBuffer(cmdBuf, buffer, dst.buffer, 1, &copyRegion);

    KL_VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuf));

    queueSubmit(queue, 0, nullptr, 0, nullptr, 1, &cmdBuf);
    KL_VK_CHECK_RESULT(vkQueueWaitIdle(queue));
}
