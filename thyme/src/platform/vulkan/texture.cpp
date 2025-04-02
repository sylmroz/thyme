#include <thyme/platform/vulkan/texture.hpp>

th::vulkan::VulkanTexture::VulkanTexture(const Device& device, const vk::UniqueCommandPool& commandPool,
                                         const Texture& texture)
    : imageMemory(ImageMemory(device,
                                    commandPool,
                                    texture.getData(),
                                    texture.getResolution(),
                                    vk::SampleCountFlagBits::e1,
                                    texture.getMipLevels())),
      sampler(createImageSampler(device, texture.getMipLevels())) {}