module;

module th.render_system.vulkan;

import th.scene.model;

namespace th {

VulkanModel::VulkanModel(const Model& model, const VulkanDevice& device)
    : m_vertex_memory_buffer{ VulkanBufferMemory(device, model.mesh.vertices, vk::BufferUsageFlagBits::eVertexBuffer) },
      m_index_memory_buffer{ VulkanBufferMemory(device, model.mesh.indices, vk::BufferUsageFlagBits::eIndexBuffer) },
      m_uniform_buffer_object{ device }, m_texture{ device, model.texture },
      m_indices_size{ static_cast<uint32_t>(model.mesh.indices.size()) } {}

void VulkanModel::draw(const vk::CommandBuffer command_buffer) const noexcept {
    command_buffer.bindVertexBuffers(0, { m_vertex_memory_buffer.getBuffer().get() }, { 0 });
    command_buffer.bindIndexBuffer(*m_index_memory_buffer.getBuffer(), 0, vk::IndexType::eUint32);
    command_buffer.drawIndexed(m_indices_size, 1, 0, 0, 0);
}

}// namespace th