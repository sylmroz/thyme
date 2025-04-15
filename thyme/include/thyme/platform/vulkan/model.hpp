#pragma once

#include <thyme/platform/vulkan/texture.hpp>
#include <thyme/platform/vulkan/uniform_buffer_object.hpp>
#include <thyme/platform/vulkan/utils.hpp>

#include <thyme/scene/model.hpp>

namespace th::vulkan {

class VulkanModel {
public:
    VulkanModel(const scene::Model& model, const Device& device, const vk::CommandPool commandPool);

    [[nodiscard]] auto getVertexMemoryBuffer() const noexcept -> const BufferMemory& {
        return m_vertexMemoryBuffer;
    }
    [[nodiscard]] auto getIndexMemoryBuffer() const noexcept -> const BufferMemory& {
        return m_indexMemoryBuffer;
    }
    [[nodiscard]] auto getUniformBufferObject() const noexcept -> const UniformBufferObject<renderer::MVP>& {
        return m_uniformBufferObject;
    }
    [[nodiscard]] auto getTexture() const noexcept -> const VulkanTexture& {
        return m_texture;
    }
    [[nodiscard]] uint32_t getIndicesSize() const noexcept {
        return m_indicesSize;
    }

    void draw(const vk::CommandBuffer commandBuffer) const noexcept;

private:
    BufferMemory m_vertexMemoryBuffer;
    BufferMemory m_indexMemoryBuffer;
    UniformBufferObject<renderer::MVP> m_uniformBufferObject;
    VulkanTexture m_texture;
    uint32_t m_indicesSize;
};

}// namespace th::vulkan
