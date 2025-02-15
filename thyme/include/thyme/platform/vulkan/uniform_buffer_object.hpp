#pragma once

#include <thyme/platform/vulkan/utils.hpp>

#include <thyme/renderer/models.hpp>


#include <array>
#include <ranges>
#include <utility>

namespace Thyme::Vulkan {

template <typename T, size_t Frames>
class UniformBufferObject {
public:
    UniformBufferObject(const Device& device) {
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

    std::array<vk::DescriptorBufferInfo, Frames> getDescriptorBufferInfos() {
        return [&]<size_t... Ints>(std::index_sequence<Ints...>) {
            return std::array{ vk::DescriptorBufferInfo(*m_uniformMemoryBuffers[Ints].buffer, 0, sizeof(T))... };
        }(std::make_index_sequence<Frames>());
    }

    void update(const uint32_t index, const T obj) const noexcept {
        memcpy(m_mappedMemoryBuffers[index], &obj, sizeof(obj));
    }

private:
    std::array<BufferMemory, Frames> m_uniformMemoryBuffers;
    std::array<void*, Frames> m_mappedMemoryBuffers;
};

}// namespace Thyme::Vulkan