export module th.render_system.vulkan:model;

import glm;
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
        return m_vertex_memory_buffer;
    }
    [[nodiscard]] auto getIndexMemoryBuffer() const noexcept -> const VulkanBufferMemory& {
        return m_index_memory_buffer;
    }
    [[nodiscard]] auto getUniformBufferObject() const noexcept -> const VulkanUniformBuffer<glm::mat4>& {
        return m_uniform_buffer_object;
    }
    [[nodiscard]] auto getTexture() const noexcept -> const Vulkan2DTexture& {
        return m_texture;
    }
    [[nodiscard]] auto getIndicesSize() const noexcept -> std::uint32_t {
        return m_indices_size;
    }

    void draw(vk::CommandBuffer command_buffer) const noexcept;

private:
    VulkanBufferMemory m_vertex_memory_buffer;
    VulkanBufferMemory m_index_memory_buffer;
    VulkanUniformBuffer<glm::mat4> m_uniform_buffer_object;
    Vulkan2DTexture m_texture;
    std::uint32_t m_indices_size;
};

}// namespace th
