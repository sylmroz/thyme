export module th.render_system.vulkan.buffer;

import vulkan_hpp;

import th.render_system.vulkan.device;
import th.render_system.vulkan.utils;

namespace th {

export class VulkanBufferMemory {
public:
    VulkanBufferMemory(const VulkanDevice& device, size_t size, vk::BufferUsageFlags usage,
                       vk::MemoryPropertyFlags properties);
    VulkanBufferMemory(vk::Device device, vk::PhysicalDevice physicalDevice, size_t size, vk::BufferUsageFlags usage,
                       vk::MemoryPropertyFlags properties);

    template <typename Vec>
    VulkanBufferMemory(const vk::Device device, vk::PhysicalDevice physicalDevice, const vk::Queue graphicQueue,
                       const vk::CommandPool commandPool, const Vec& data, const vk::BufferUsageFlags usage)
        : VulkanBufferMemory(device, physicalDevice, data.size() * sizeof(data[0]),
                             vk::BufferUsageFlagBits::eTransferDst | usage, vk::MemoryPropertyFlagBits::eDeviceLocal) {
        const auto size = data.size() * sizeof(data[0]);

        const auto stagingMemoryBuffer = VulkanBufferMemory(device,
                                                            physicalDevice,
                                                            size,
                                                            vk::BufferUsageFlagBits::eTransferSrc,
                                                            vk::MemoryPropertyFlagBits::eHostVisible
                                                                    | vk::MemoryPropertyFlagBits::eHostCoherent);

        void* mappedMemory = nullptr;
        [[maybe_unused]] const auto result =
                device.mapMemory(stagingMemoryBuffer.getMemory().get(), 0, size, vk::MemoryMapFlags(), &mappedMemory);
        memcpy(mappedMemory, data.data(), size);
        device.unmapMemory(stagingMemoryBuffer.getMemory().get());
        copyBuffer(device, commandPool, graphicQueue, stagingMemoryBuffer.getBuffer().get(), m_buffer.get(), size);
    }

    template <typename Vec>
    VulkanBufferMemory(const VulkanDevice& device, const Vec& data, const vk::BufferUsageFlags usage)
        : VulkanBufferMemory(device.logicalDevice, device.physicalDevice, device.getGraphicQueue(), device.commandPool,
                             data, usage) {}

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
