export module th.render_system.vulkan:buffer;

import std;

import vulkan_hpp;

import :device;
import :utils;

namespace th {

export class VulkanBufferMemory {
public:
    VulkanBufferMemory(const VulkanDevice& device, size_t size, vk::BufferUsageFlags usage,
                       vk::MemoryPropertyFlags properties);
    VulkanBufferMemory(vk::Device device, vk::PhysicalDevice physical_device, size_t size, vk::BufferUsageFlags usage,
                       vk::MemoryPropertyFlags properties);

    template <typename Vec>
    VulkanBufferMemory(const vk::Device device, const vk::PhysicalDevice physical_device, const vk::Queue graphic_queue,
                       const vk::CommandPool command_pool, const Vec& data, const vk::BufferUsageFlags usage)
        : VulkanBufferMemory(device, physical_device, data.size() * sizeof(data[0]),
                             vk::BufferUsageFlagBits::eTransferDst | usage, vk::MemoryPropertyFlagBits::eDeviceLocal) {
        const auto size = data.size() * sizeof(data[0]);

        const auto staging_memory_buffer = VulkanBufferMemory(device,
                                                              physical_device,
                                                              size,
                                                              vk::BufferUsageFlagBits::eTransferSrc,
                                                              vk::MemoryPropertyFlagBits::eHostVisible
                                                                      | vk::MemoryPropertyFlagBits::eHostCoherent);

        void* mapped_memory = nullptr;
        [[maybe_unused]] const auto result = device.mapMemory(
                staging_memory_buffer.getMemory().get(), 0, size, vk::MemoryMapFlags(), &mapped_memory);
        std::memcpy(mapped_memory, data.data(), size);
        device.unmapMemory(staging_memory_buffer.getMemory().get());
        copyBuffer(device, command_pool, graphic_queue, staging_memory_buffer.getBuffer().get(), m_buffer.get(), size);
    }

    template <typename Vec>
    VulkanBufferMemory(const VulkanDevice& device, const Vec& data, const vk::BufferUsageFlags usage)
        : VulkanBufferMemory(device.logical_device, device.physical_device, device.getGraphicQueue(),
                             device.command_pool, data, usage) {}

    [[nodiscard]] auto getBuffer() const noexcept -> const vk::UniqueBuffer& {
        return m_buffer;
    }
    [[nodiscard]] auto getMemory() const noexcept -> const vk::UniqueDeviceMemory& {
        return m_memory;
    }

private:
    vk::UniqueBuffer m_buffer;
    vk::UniqueDeviceMemory m_memory;
};

}// namespace th
