#include <thyme/platform/vulkan/texture.hpp>

namespace th::vulkan {

VulkanTexture::VulkanTexture(const Device& device, const vk::UniqueCommandPool& commandPool, const Texture& texture)
    : m_imageMemory{ ImageMemory(device,
                               commandPool,
                               texture.getData(),
                               texture.getResolution(),
                               vk::SampleCountFlagBits::e1,
                               texture.getMipLevels()) },
      m_sampler{ createImageSampler(device, texture.getMipLevels()) } {}

}// namespace th::vulkan