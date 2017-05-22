/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "VulkanBuffer.h"

vk::Buffer::Buffer(VkDevice device, VkDeviceSize size, uint32_t flags, VkPhysicalDeviceMemoryProperties memProps):
    device(device),
    size(size)
{
    VkBufferUsageFlags usage = 0;
    VkMemoryPropertyFlags propFlags = 0;

    if (flags & Host)
        propFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    if (flags & Device)
        propFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (flags & Vertex)
        usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (flags & Index)
        usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (flags & Uniform)
        usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (flags & TransferSrc)
        usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (flags & TransferDst)
        usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
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
    allocInfo.memoryTypeIndex = findMemoryType(memProps, memReqs.memoryTypeBits, propFlags);

    memory = Resource<VkDeviceMemory>{device, vkFreeMemory};
    KL_VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, memory.cleanRef()));
    KL_VK_CHECK_RESULT(vkBindBufferMemory(device, buffer, memory, 0));
}

vk::Buffer::Buffer(Buffer &&other) noexcept
{
    swap(other);
}

auto vk::Buffer::operator=(Buffer other) noexcept -> Buffer&
{
    swap(other);
    return *this;
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
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = cmdPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmdBuf;
    KL_VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, &cmdBuf));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    KL_VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuf, &beginInfo));

    VkBufferCopy copyRegion{};
    copyRegion.size = dst.size;
    vkCmdCopyBuffer(cmdBuf, buffer, dst.buffer, 1, &copyRegion);

    KL_VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuf));

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;

    KL_VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, nullptr));
    KL_VK_CHECK_RESULT(vkQueueWaitIdle(queue));

    vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuf);
}

void vk::Buffer::swap(Buffer &other) noexcept
{
    std::swap(device, other.device);
    std::swap(memory, other.memory);
    std::swap(buffer, other.buffer);
    std::swap(size, other.size);
}
