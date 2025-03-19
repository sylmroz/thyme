#pragma once

#include <thyme/platform/vulkan/utils.hpp>

#include <utility>

namespace th::vulkan {

template <typename T>
class UniformBufferObject final: NoCopyable {
public:
    explicit UniformBufferObject(const Device& device) : m_device{ device } {
        constexpr auto ubSize = sizeof(T);
        m_uniformMemoryBuffer = createBufferMemory(device,
                                                   ubSize,
                                                   vk::BufferUsageFlagBits::eUniformBuffer,
                                                   vk::MemoryPropertyFlagBits::eHostVisible
                                                           | vk::MemoryPropertyFlagBits::eHostCoherent);
        [[maybe_unused]] const auto result = device.logicalDevice->mapMemory(
                *m_uniformMemoryBuffer.memory, 0, ubSize, vk::MemoryMapFlags(), &m_mappedMemoryBuffer);
    }

    [[nodiscard]] auto getDescriptorBufferInfos() const noexcept -> vk::DescriptorBufferInfo {
        return vk::DescriptorBufferInfo(*m_uniformMemoryBuffer.buffer, 0, sizeof(T));
    }

    void update(const T& obj) const noexcept {
        memcpy(m_mappedMemoryBuffer, &obj, sizeof(obj));
    }

    ~UniformBufferObject() override {
        m_device.logicalDevice->unmapMemory(*m_uniformMemoryBuffer.memory);
    }

private:
    BufferMemory m_uniformMemoryBuffer;
    void* m_mappedMemoryBuffer{ nullptr };

    const Device& m_device;
};

}// namespace th::vulkan