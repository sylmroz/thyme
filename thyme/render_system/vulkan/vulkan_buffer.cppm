export module th.render_system.vulkan:buffer;

import std;

import vulkan;

import :device;
import :utils;

namespace th {

export class VulkanBufferMemory {
public:
    VulkanBufferMemory(const VulkanDeviceRAII& device, size_t size, vk::BufferUsageFlags usage,
                       vk::MemoryPropertyFlags memory_property_flags);
    VulkanBufferMemory(const vk::raii::Device& device, vk::PhysicalDevice physical_device, size_t size,
                       vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memory_property_flags);

    template <typename Vec>
    VulkanBufferMemory(const vk::raii::Device& device, const vk::PhysicalDevice physical_device,
                       const vk::Queue graphic_queue, const vk::CommandPool command_pool, const Vec& data,
                       const vk::BufferUsageFlags usage)
        : VulkanBufferMemory(device, physical_device, data.size() * sizeof(data[0]),
                             vk::BufferUsageFlagBits::eTransferDst | usage, vk::MemoryPropertyFlagBits::eDeviceLocal) {
        const auto size = data.size() * sizeof(data[0]);

        const auto staging_memory_buffer = VulkanBufferMemory(device,
                                                              physical_device,
                                                              size,
                                                              vk::BufferUsageFlagBits::eTransferSrc,
                                                              vk::MemoryPropertyFlagBits::eHostVisible
                                                                      | vk::MemoryPropertyFlagBits::eHostCoherent);

        void* mapped_memory = staging_memory_buffer.getMemory().mapMemory(0, size, vk::MemoryMapFlags());
        std::memcpy(mapped_memory, data.data(), size);
        staging_memory_buffer.getMemory().unmapMemory();

        copyBuffer(device, command_pool, graphic_queue, staging_memory_buffer.getBuffer(), m_buffer, size);
    }

    template <typename Vec>
    VulkanBufferMemory(const VulkanDeviceRAII& device, const Vec& data, const vk::BufferUsageFlags usage)
        : VulkanBufferMemory(device.logical_device, device.physical_device, device.getGraphicQueue(),
                             device.command_pool, data, usage) {}

    [[nodiscard]] auto getBuffer() const noexcept -> const vk::raii::Buffer& {
        return m_buffer;
    }
    [[nodiscard]] auto getMemory() const noexcept -> const vk::raii::DeviceMemory& {
        return m_memory;
    }

    void unmapMemory() const noexcept {
        if (*m_memory != nullptr) {
            m_memory.unmapMemory();
        }
    }

private:
    vk::raii::Buffer m_buffer;
    vk::raii::DeviceMemory m_memory;
};

}// namespace th
