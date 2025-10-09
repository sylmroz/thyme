module;

module th.render_system.vulkan;

namespace th {

VulkanBufferMemory::VulkanBufferMemory(const VulkanDevice& device, const size_t size, const vk::BufferUsageFlags usage,
                                       const vk::MemoryPropertyFlags properties)
    : VulkanBufferMemory(device.logical_device, device.physical_device, size, usage, properties) {}

VulkanBufferMemory::VulkanBufferMemory(const vk::Device device, const vk::PhysicalDevice physical_device,
                                       const size_t size, const vk::BufferUsageFlags usage,
                                       const vk::MemoryPropertyFlags properties) {
    m_buffer = device.createBufferUnique(vk::BufferCreateInfo{
            .size = size,
            .usage = usage,
            .sharingMode = vk::SharingMode::eExclusive,
    });

    vk::MemoryRequirements memory_requirements;
    device.getBufferMemoryRequirements(m_buffer.get(), &memory_requirements);
    m_memory = device.allocateMemoryUnique(vk::MemoryAllocateInfo{
            .allocationSize = memory_requirements.size,
            .memoryTypeIndex = findMemoryType(physical_device, memory_requirements.memoryTypeBits, properties),
    });

    device.bindBufferMemory(m_buffer.get(), m_memory.get(), 0);
}

}// namespace th
