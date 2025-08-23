export module th.render_system.vulkan:uniform_buffer_object;

import vulkan_hpp;

import :buffer;
import :device;

export namespace th {

template <typename T>
class VulkanUniformBuffer final {
public:
    explicit VulkanUniformBuffer(const VulkanDevice& device)
        : m_uniformMemoryBuffer{ VulkanBufferMemory(device,
                                              sizeof(T),
                                              vk::BufferUsageFlagBits::eUniformBuffer,
                                              vk::MemoryPropertyFlagBits::eHostVisible
                                                      | vk::MemoryPropertyFlagBits::eHostCoherent) },
          m_device{ device.logicalDevice } {
        [[maybe_unused]] const auto result = device.logicalDevice.mapMemory(
                *m_uniformMemoryBuffer.getMemory(), 0, sizeof(T), vk::MemoryMapFlags(), &m_mappedMemoryBuffer);
    }

    explicit VulkanUniformBuffer(const VulkanUniformBuffer&) = delete;
    explicit VulkanUniformBuffer(VulkanUniformBuffer&&) = default;
    auto operator=(const VulkanUniformBuffer&) -> VulkanUniformBuffer& = delete;
    auto operator=(VulkanUniformBuffer&&) -> VulkanUniformBuffer& = default;

    [[nodiscard]] auto getDescriptorBufferInfos() const noexcept -> vk::DescriptorBufferInfo {
        return vk::DescriptorBufferInfo(*m_uniformMemoryBuffer.getBuffer(), 0, sizeof(T));
    }

    void update(const T& obj) const noexcept {
        memcpy(m_mappedMemoryBuffer, &obj, sizeof(obj));
    }

    ~VulkanUniformBuffer() {
        if (m_uniformMemoryBuffer.getMemory().get() != nullptr) {
            m_device.unmapMemory(m_uniformMemoryBuffer.getMemory().get());
        }
    }

private:
    VulkanBufferMemory m_uniformMemoryBuffer;
    void* m_mappedMemoryBuffer{ nullptr };

    vk::Device m_device;
};

}// namespace th::render_system::vulkan
