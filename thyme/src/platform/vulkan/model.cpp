#include <thyme/platform/vulkan/model.hpp>
#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/platform/vulkan/vulkan_buffer.hpp>

namespace th::vulkan {

VulkanModel::VulkanModel(const scene::Model& model, const VulkanDevice& device)
    : m_vertexMemoryBuffer{ BufferMemory(device, model.mesh.vertices,
                                         vk::BufferUsageFlagBits::eVertexBuffer) },
      m_indexMemoryBuffer{ BufferMemory(device, model.mesh.indices,
                                        vk::BufferUsageFlagBits::eIndexBuffer) },
      m_uniformBufferObject{ device }, m_texture{ device, model.texture },
      m_indicesSize{ static_cast<uint32_t>(model.mesh.indices.size()) } {}

void VulkanModel::draw(const vk::CommandBuffer commandBuffer) const noexcept {
    commandBuffer.bindVertexBuffers(0, { m_vertexMemoryBuffer.getBuffer().get() }, { 0 });
    commandBuffer.bindIndexBuffer(*m_indexMemoryBuffer.getBuffer(), 0, vk::IndexType::eUint32);
    commandBuffer.drawIndexed(m_indicesSize, 1, 0, 0, 0);
}

}// namespace th::vulkan
