#pragma once

#include <thyme/scene/texture.hpp>
#include <thyme/platform/vulkan/utils.hpp>

#include <vulkan/vulkan.hpp>

namespace th::vulkan {

class VulkanTexture {
public:
    VulkanTexture(const Device& device, const vk::UniqueCommandPool& commandPool, const Texture& texture);

    [[nodiscard]] auto getImage() const noexcept -> const vk::UniqueImage& {
        return m_imageMemory.getImage();
    }
    [[nodiscard]] auto getImageView() const noexcept -> const vk::UniqueImageView& {
        return m_imageMemory.getImageView();
    }
    [[nodiscard]] auto getSampler() const noexcept -> const vk::UniqueSampler& {
        return m_sampler;
    }
    [[nodiscard]] auto getImageMemory() const noexcept -> const ImageMemory& {
        return m_imageMemory;
    }

private:
    ImageMemory m_imageMemory;
    vk::UniqueSampler m_sampler;
};

}// namespace th::vulkan