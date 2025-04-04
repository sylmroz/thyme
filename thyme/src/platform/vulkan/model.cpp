#include <thyme/platform/vulkan/model.hpp>
#include <thyme/platform/vulkan/utils.hpp>

namespace th::vulkan {

VulkanModel::VulkanModel(const scene::Model& model, const Device& device, const vk::UniqueCommandPool& commandPool)
    : m_vertexMemoryBuffer{ BufferMemory(device, commandPool, model.mesh.vertices,
                                               vk::BufferUsageFlagBits::eVertexBuffer) },
      m_indexMemoryBuffer{ BufferMemory(device, commandPool, model.mesh.indices,
                                              vk::BufferUsageFlagBits::eIndexBuffer) },
      m_uniformBufferObject{ device }, m_texture{ device, commandPool, model.texture },
      indicesSize{ static_cast<uint32_t>(model.mesh.indices.size()) } {}

void VulkanModel::draw(const vk::CommandBuffer commandBuffer) const noexcept {
    commandBuffer.bindVertexBuffers(0, { *m_vertexMemoryBuffer.getBuffer() }, { 0 });
    commandBuffer.bindIndexBuffer(*m_indexMemoryBuffer.getBuffer(), 0, vk::IndexType::eUint32);
    commandBuffer.drawIndexed(indicesSize, 1, 0, 0, 0);
}

}// namespace th::vulkan
