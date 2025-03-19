#pragma once

#include <thyme/core/texture.hpp>
#include <thyme/platform/vulkan/utils.hpp>

#include <vulkan/vulkan.hpp>

namespace th::vulkan {

class VulkanTexture {
public:
    VulkanTexture(const Device& device, const vk::UniqueCommandPool& commandPool, const Texture& texture);

    ImageMemory imageMemory;
    vk::UniqueSampler sampler;
};

}// namespace th::vulkan