#include <thyme\platform\vulkan\model.hpp>
#include <thyme\platform\vulkan\utils.hpp>

namespace th::vulkan {
VulkanModel::VulkanModel(const renderer::Model& model, const Device& device, const vk::UniqueCommandPool& commandPool)
    : m_vertexMemoryBuffer{ createBufferMemory(device, commandPool, model.vertices,
                                               vk::BufferUsageFlagBits::eVertexBuffer) },
      m_indexMemoryBuffer{ createBufferMemory(device, commandPool, model.indices,
                                              vk::BufferUsageFlagBits::eIndexBuffer) },
      m_uniformBufferObject{ device }, m_texture{ device, commandPool, model.texture } {}
}// namespace th::vulkan