module;

module th.render_system.vulkan;

namespace th {

vk::raii::DeviceMemory initializeMemory(const vk::raii::Device& device, const vk::PhysicalDevice physical_device,
                                        const vk::MemoryPropertyFlags memory_property_flags,
                                        const vk::MemoryRequirements& memory_requirements) {
    return device.allocateMemory(vk::MemoryAllocateInfo{
            .allocationSize = memory_requirements.size,
            .memoryTypeIndex = findMemoryType(physical_device, memory_requirements.memoryTypeBits, memory_property_flags),
    });
}

VulkanBufferMemory::VulkanBufferMemory(const VulkanDeviceRAII& device, const size_t size,
                                       const vk::BufferUsageFlags usage, const vk::MemoryPropertyFlags properties)
    : VulkanBufferMemory(device.logical_device, device.physical_device, size, usage, properties) {}

VulkanBufferMemory::VulkanBufferMemory(const vk::raii::Device& device, const vk::PhysicalDevice physical_device,
                                       const size_t size, const vk::BufferUsageFlags usage,
                                       const vk::MemoryPropertyFlags memory_property_flags)
    : m_buffer(device.createBuffer(vk::BufferCreateInfo{
              .size = size,
              .usage = usage,
              .sharingMode = vk::SharingMode::eExclusive,
      })),
      m_memory(initializeMemory(device, physical_device, memory_property_flags, m_buffer.getMemoryRequirements())) {
    m_buffer.bindMemory(m_memory, 0);
}

}// namespace th
