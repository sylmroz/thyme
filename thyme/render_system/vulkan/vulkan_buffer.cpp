module;

module th.render_system.vulkan;

namespace th {

VulkanBufferMemory::VulkanBufferMemory(const VulkanDevice& device, const size_t size, const vk::BufferUsageFlags usage,
                                       const vk::MemoryPropertyFlags properties)
    : VulkanBufferMemory(device.logicalDevice, device.physicalDevice, size, usage, properties) {}

VulkanBufferMemory::VulkanBufferMemory(const vk::Device device, const vk::PhysicalDevice physicalDevice,
                                       const size_t size, const vk::BufferUsageFlags usage,
                                       const vk::MemoryPropertyFlags properties) {
    m_buffer = device.createBufferUnique(vk::BufferCreateInfo{
            .size = size,
            .usage = usage,
            .sharingMode = vk::SharingMode::eExclusive,
    });

    vk::MemoryRequirements memoryRequirements;
    device.getBufferMemoryRequirements(m_buffer.get(), &memoryRequirements);
    m_memory = device.allocateMemoryUnique(vk::MemoryAllocateInfo{
            .allocationSize = memoryRequirements.size,
            .memoryTypeIndex = findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, properties),
    });

    device.bindBufferMemory(m_buffer.get(), m_memory.get(), 0);
}

}// namespace th
