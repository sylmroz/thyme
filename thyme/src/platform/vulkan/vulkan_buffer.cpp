#include <thyme/platform/vulkan/vulkan_buffer.hpp>

namespace th::vulkan {

BufferMemory::BufferMemory(const VulkanDevice& device, const size_t size, const vk::BufferUsageFlags usage,
                           const vk::MemoryPropertyFlags properties)
    : BufferMemory(device.logicalDevice, device.physicalDevice, size, usage, properties) {}

BufferMemory::BufferMemory(const vk::Device device, const vk::PhysicalDevice physicalDevice, const size_t size,
                           const vk::BufferUsageFlags usage, const vk::MemoryPropertyFlags properties) {
    m_buffer = device.createBufferUnique(
            vk::BufferCreateInfo(vk::BufferCreateFlagBits(), size, usage, vk::SharingMode::eExclusive));

    vk::MemoryRequirements memoryRequirements;
    device.getBufferMemoryRequirements(m_buffer.get(), &memoryRequirements);
    m_memory = device.allocateMemoryUnique(vk::MemoryAllocateInfo(
            memoryRequirements.size, findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, properties)));

    device.bindBufferMemory(m_buffer.get(), m_memory.get(), 0);
}

}// namespace th::vulkan