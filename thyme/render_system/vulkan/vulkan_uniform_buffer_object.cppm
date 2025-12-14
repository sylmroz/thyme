export module th.render_system.vulkan:uniform_buffer_object;

import std;

import vulkan;

import :buffer;
import :device;

export namespace th {

template <typename T>
class VulkanUniformBuffer final {
public:
    explicit VulkanUniformBuffer(const VulkanDeviceRAII& device)
        : m_uniformMemoryBuffer{ VulkanBufferMemory(device,
                                                    sizeof(T),
                                                    vk::BufferUsageFlagBits::eUniformBuffer,
                                                    vk::MemoryPropertyFlagBits::eHostVisible
                                                            | vk::MemoryPropertyFlagBits::eHostCoherent) },
          m_mappedMemoryBuffer{ m_uniformMemoryBuffer.getMemory().mapMemory(0, sizeof(T), vk::MemoryMapFlags()) }

    {}

    explicit VulkanUniformBuffer(const VulkanUniformBuffer&) = delete;
    explicit VulkanUniformBuffer(VulkanUniformBuffer&&) = default;
    auto operator=(const VulkanUniformBuffer&) -> VulkanUniformBuffer& = delete;
    auto operator=(VulkanUniformBuffer&&) -> VulkanUniformBuffer& = default;

    [[nodiscard]] auto getDescriptorBufferInfos() const noexcept -> vk::DescriptorBufferInfo {
        return vk::DescriptorBufferInfo(*m_uniformMemoryBuffer.getBuffer(), 0, sizeof(T));
    }

    void update(const T& obj) const noexcept {
        std::memcpy(m_mappedMemoryBuffer, &obj, sizeof(obj));
    }

    ~VulkanUniformBuffer() {
        m_uniformMemoryBuffer.unmapMemory();
    }

private:
    VulkanBufferMemory m_uniformMemoryBuffer;
    void* m_mappedMemoryBuffer{ nullptr };
};

}// namespace th
