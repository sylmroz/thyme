#pragma once

#include <thyme/platform/vulkan/texture.hpp>
#include <thyme/platform/vulkan/uniform_buffer_object.hpp>
#include <thyme/platform/vulkan/utils.hpp>

#include <thyme/renderer/model.hpp>

namespace th::vulkan {
class VulkanModel {
public:
    explicit VulkanModel(const renderer::Model& model, const Device& device, const vk::UniqueCommandPool& commandPool);

private:
    BufferMemory m_vertexMemoryBuffer;
    BufferMemory m_indexMemoryBuffer;
    UniformBufferObject<renderer::MVP> m_uniformBufferObject;
    VulkanTexture m_texture;
};
}// namespace th::vulkan
