#ifndef THYME_PLATFORM_VULKAN_VULKAN_BUFFER_HPP
#define THYME_PLATFORM_VULKAN_VULKAN_BUFFER_HPP

#include <vulkan/vulkan.hpp>

#include <thyme/platform/vulkan/vulkan_device.hpp>

namespace th::vulkan {

class BufferMemory {
public:
    BufferMemory(const VulkanDevice& device, size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    BufferMemory(vk::Device device, vk::PhysicalDevice physicalDevice, size_t size, vk::BufferUsageFlags usage,
                 vk::MemoryPropertyFlags properties);

    template <typename Vec>
    BufferMemory(const vk::Device device, vk::PhysicalDevice physicalDevice, const vk::Queue graphicQueue,
                 const vk::CommandPool commandPool, const Vec& data, const vk::BufferUsageFlags usage)
        : BufferMemory(device, physicalDevice, data.size() * sizeof(data[0]),
                       vk::BufferUsageFlagBits::eTransferDst | usage, vk::MemoryPropertyFlagBits::eDeviceLocal) {
        const auto size = data.size() * sizeof(data[0]);

        const auto stagingMemoryBuffer =
                BufferMemory(device,
                             physicalDevice,
                             size,
                             vk::BufferUsageFlagBits::eTransferSrc,
                             vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        void* mappedMemory = nullptr;
        [[maybe_unused]] const auto result =
                device.mapMemory(stagingMemoryBuffer.getMemory().get(), 0, size, vk::MemoryMapFlags(), &mappedMemory);
        memcpy(mappedMemory, data.data(), size);
        device.unmapMemory(stagingMemoryBuffer.getMemory().get());
        copyBuffer(device, commandPool, graphicQueue, stagingMemoryBuffer.getBuffer().get(), m_buffer.get(), size);
    }

    template <typename Vec>
    BufferMemory(const VulkanDevice& device, const Vec& data, const vk::BufferUsageFlags usage)
        : BufferMemory(device.logicalDevice, device.physicalDevice, device.getGraphicQueue(),
                       device.commandPool, data, usage) {}

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

}
#endif
