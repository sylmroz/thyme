#pragma once

#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/scene/texture.hpp>

#include <vulkan/vulkan.hpp>

namespace th::vulkan {

class VulkanTexture {
public:
    VulkanTexture(const Device& device, const vk::CommandPool commandPool, const Texture& texture);

    [[nodiscard]] auto getImage() const noexcept -> vk::Image {
        return m_imageMemory.getImage();
    }
    [[nodiscard]] auto getImageView() const noexcept -> vk::ImageView {
        return m_imageMemory.getImageView();
    }
    [[nodiscard]] auto getSampler() const noexcept -> vk::Sampler {
        return m_sampler.get();
    }
    [[nodiscard]] auto getImageMemory() const noexcept -> const ImageMemory& {
        return m_imageMemory;
    }
    [[nodiscard]] auto getDescriptorImageInfo(const vk::ImageLayout imageLayout) const noexcept -> vk::DescriptorImageInfo {
        return {m_sampler.get(), getImageView(), imageLayout};
    }

private:
    ImageMemory m_imageMemory;
    vk::UniqueSampler m_sampler;
};

}// namespace th::vulkan