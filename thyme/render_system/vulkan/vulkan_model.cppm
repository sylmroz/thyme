module;

#include <glm/glm.hpp>

export module th.render_system.vulkan:model;

import vulkan_hpp;

import th.scene.model;

import :buffer;
import :device;
import :uniform_buffer_object;
import :texture;


namespace th {

export class VulkanModel {
public:
    VulkanModel(const Model& model, const VulkanDevice& device);

    [[nodiscard]] auto getVertexMemoryBuffer() const noexcept -> const VulkanBufferMemory& {
        return m_vertexMemoryBuffer;
    }
    [[nodiscard]] auto getIndexMemoryBuffer() const noexcept -> const VulkanBufferMemory& {
        return m_indexMemoryBuffer;
    }
    [[nodiscard]] auto getUniformBufferObject() const noexcept -> const VulkanUniformBuffer<glm::mat4>& {
        return m_uniformBufferObject;
    }
    [[nodiscard]] auto getTexture() const noexcept -> const Vulkan2DTexture& {
        return m_texture;
    }
    [[nodiscard]] auto getIndicesSize() const noexcept -> std::uint32_t {
        return m_indicesSize;
    }

    void draw(vk::CommandBuffer commandBuffer) const noexcept;

private:
    VulkanBufferMemory m_vertexMemoryBuffer;
    VulkanBufferMemory m_indexMemoryBuffer;
    VulkanUniformBuffer<glm::mat4> m_uniformBufferObject;
    Vulkan2DTexture m_texture;
    std::uint32_t m_indicesSize;
};

}// namespace th
