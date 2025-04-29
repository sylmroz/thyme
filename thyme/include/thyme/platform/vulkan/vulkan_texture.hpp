#pragma once

#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/scene/vulkan_texture.hpp>

#include <vulkan/vulkan.hpp>

namespace th::vulkan {

class ImageMemory {
public:
    ImageMemory(const Device& device, Resolution resolution, vk::Format format, vk::ImageUsageFlags imageUsageFlags,
                vk::MemoryPropertyFlags memoryPropertyFlags, vk::ImageAspectFlags aspectFlags,
                vk::SampleCountFlagBits msaa, uint32_t mipLevels);

    [[nodiscard]] auto getImage() const noexcept -> vk::Image {
        return m_image.get();
    }

    [[nodiscard]] auto getMemory() const noexcept -> vk::DeviceMemory {
        return m_memory.get();
    }

    [[nodiscard]] auto getImageView() const noexcept -> vk::ImageView {
        return m_imageView.get();
    }

    void resize(Resolution resolution);

private:
    vk::UniqueImage m_image;
    vk::UniqueDeviceMemory m_memory;
    vk::UniqueImageView m_imageView;

    vk::Device m_device;
    vk::PhysicalDevice m_physicalDevice;
    vk::CommandPool m_commandPool;

    vk::Format m_format;
    vk::ImageUsageFlags m_imageUsageFlags;
    vk::MemoryPropertyFlags m_memoryPropertyFlags;
    vk::ImageAspectFlags m_aspectFlags;
    vk::SampleCountFlagBits m_msaa;
    uint32_t m_mipLevels{1};
    Resolution m_resolution{};
};

class Vulkan2DTexture {
public:
    Vulkan2DTexture(const Device& device, const TextureData& texture);

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
    [[nodiscard]] auto getDescriptorImageInfo(const vk::ImageLayout imageLayout) const noexcept
            -> vk::DescriptorImageInfo {
        return { m_sampler.get(), getImageView(), imageLayout };
    }

    void setData(const TextureData& texture);

private:
    ImageMemory m_imageMemory;
    vk::UniqueSampler m_sampler;

    vk::Device m_device;
    vk::PhysicalDevice m_physicalDevice;
};

}// namespace th::vulkan