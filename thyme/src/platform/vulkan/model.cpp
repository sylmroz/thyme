#include <thyme/platform/vulkan/model.hpp>
#include <thyme/platform/vulkan/utils.hpp>

namespace th::vulkan {

VulkanModel::VulkanModel(const scene::Model& model, const Device& device, const vk::UniqueCommandPool& commandPool)
    : m_vertexMemoryBuffer{ createBufferMemory(device, commandPool, model.mesh.vertices,
                                               vk::BufferUsageFlagBits::eVertexBuffer) },
      m_indexMemoryBuffer{ createBufferMemory(device, commandPool, model.mesh.indices,
                                              vk::BufferUsageFlagBits::eIndexBuffer) },
      m_uniformBufferObject{ device }, m_texture{ device, commandPool, model.texture },
      indicesSize{ static_cast<uint32_t>(model.mesh.indices.size()) } {}
}// namespace th::vulkan
