#pragma once

#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/renderer/models.hpp>

#include <array>
#include <ranges>
#include <utility>

namespace Thyme::Vulkan {

/*template <typename T, size_t Frames>
class UniformBufferObject final: NoCopyable {
public:
    explicit UniformBufferObject(const Device& device) : m_device{ device } {
        for (auto memoryBufferMap : std::views::zip(m_uniformMemoryBuffers, m_mappedMemoryBuffers)) {
            constexpr auto ubSize = sizeof(T);
            auto& memoryBuffer = std::get<0>(memoryBufferMap);
            memoryBuffer = createBufferMemory(device,
                                              ubSize,
                                              vk::BufferUsageFlagBits::eUniformBuffer,
                                              vk::MemoryPropertyFlagBits::eHostVisible
                                                      | vk::MemoryPropertyFlagBits::eHostCoherent);
            [[maybe_unused]] const auto result = device.logicalDevice->mapMemory(
                    *memoryBuffer.memory, 0, ubSize, vk::MemoryMapFlags(), &std::get<1>(memoryBufferMap));
        }
    }

    [[nodiscard]] auto getDescriptorBufferInfos() const noexcept -> std::array<vk::DescriptorBufferInfo, Frames> {
        return [&]<size_t... Ints>(std::index_sequence<Ints...>) {
            return std::array{ vk::DescriptorBufferInfo(*m_uniformMemoryBuffers[Ints].buffer, 0, sizeof(T))... };
        }(std::make_index_sequence<Frames>());
    }

    void update(const uint32_t index, const T obj) const noexcept {
        memcpy(m_mappedMemoryBuffers[index], &obj, sizeof(obj));
    }

    ~UniformBufferObject() override {
        for (const auto& uniformMemory : m_uniformMemoryBuffers) {
            m_device.logicalDevice->unmapMemory(*uniformMemory.memory);
        }
    }

private:
    std::array<BufferMemory, Frames> m_uniformMemoryBuffers;
    std::array<void*, Frames> m_mappedMemoryBuffers;

    const Device& m_device;
};*/

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

}// namespace Thyme::Vulkan