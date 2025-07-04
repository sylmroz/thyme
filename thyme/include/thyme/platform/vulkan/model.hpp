#pragma once

#include <thyme/platform/vulkan/uniform_buffer_object.hpp>
#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/platform/vulkan/vulkan_buffer.hpp>
#include <thyme/platform/vulkan/vulkan_device.hpp>
#include <thyme/platform/vulkan/vulkan_texture.hpp>

#include <thyme/scene/model.hpp>

namespace th::vulkan {

class VulkanModel {
public:
    VulkanModel(const scene::Model& model, const VulkanDevice& device);

    [[nodiscard]] auto getVertexMemoryBuffer() const noexcept -> const BufferMemory& {
        return m_vertexMemoryBuffer;
    }
    [[nodiscard]] auto getIndexMemoryBuffer() const noexcept -> const BufferMemory& {
        return m_indexMemoryBuffer;
    }
    [[nodiscard]] auto getUniformBufferObject() const noexcept -> const UniformBufferObject<glm::mat4>& {
        return m_uniformBufferObject;
    }
    [[nodiscard]] auto getTexture() const noexcept -> const Vulkan2DTexture& {
        return m_texture;
    }
    [[nodiscard]] uint32_t getIndicesSize() const noexcept {
        return m_indicesSize;
    }

    void draw(vk::CommandBuffer commandBuffer) const noexcept;

private:
    BufferMemory m_vertexMemoryBuffer;
    BufferMemory m_indexMemoryBuffer;
    UniformBufferObject<glm::mat4> m_uniformBufferObject;
    Vulkan2DTexture m_texture;
    uint32_t m_indicesSize;
};

}// namespace th::vulkan
