#pragma once

#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/platform/vulkan/vulkan_buffer.hpp>
#include <thyme/platform/vulkan/vulkan_device.hpp>

#include <utility>

namespace th::vulkan {

template <typename T>
class UniformBufferObject final {
public:
    explicit UniformBufferObject(const VulkanDevice& device)
        : m_uniformMemoryBuffer{ BufferMemory(device,
                                              sizeof(T),
                                              vk::BufferUsageFlagBits::eUniformBuffer,
                                              vk::MemoryPropertyFlagBits::eHostVisible
                                                      | vk::MemoryPropertyFlagBits::eHostCoherent) },
          m_device{ device.logicalDevice } {
        [[maybe_unused]] const auto result = device.logicalDevice.mapMemory(
                *m_uniformMemoryBuffer.getMemory(), 0, sizeof(T), vk::MemoryMapFlags(), &m_mappedMemoryBuffer);
    }

    explicit UniformBufferObject(const UniformBufferObject&) = delete;
    explicit UniformBufferObject(UniformBufferObject&&) = default;
    UniformBufferObject& operator=(const UniformBufferObject&) = delete;
    UniformBufferObject& operator=(UniformBufferObject&&) = default;

    [[nodiscard]] auto getDescriptorBufferInfos() const noexcept -> vk::DescriptorBufferInfo {
        return vk::DescriptorBufferInfo(*m_uniformMemoryBuffer.getBuffer(), 0, sizeof(T));
    }

    void update(const T& obj) const noexcept {
        memcpy(m_mappedMemoryBuffer, &obj, sizeof(obj));
    }

    ~UniformBufferObject() {
        if (m_uniformMemoryBuffer.getMemory().get() != nullptr) {
            m_device.unmapMemory(m_uniformMemoryBuffer.getMemory().get());
        }
    }

private:
    BufferMemory m_uniformMemoryBuffer;
    void* m_mappedMemoryBuffer{ nullptr };

    vk::Device m_device;
};

}// namespace th::vulkan