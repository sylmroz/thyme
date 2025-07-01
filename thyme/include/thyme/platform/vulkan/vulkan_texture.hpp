#pragma once

#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/platform/vulkan/vulkan_device.hpp>
#include <thyme/scene/texture_data.hpp>

#include <vulkan/vulkan.hpp>

namespace th::vulkan {

void copyImage(vk::CommandBuffer commandBuffer, vk::Image srcImage, vk::Extent3D srcResolution, vk::Image dstImage);
void blitImage(vk::CommandBuffer commandBuffer, vk::Image srcImage, vk::Extent3D srcResolution, vk::Image dstImage, vk::Extent3D dstResolution);

class ImageMemory {
public:
    ImageMemory(const VulkanDevice& device, vk::Extent3D resolution, vk::Format format,
                vk::ImageUsageFlags imageUsageFlags, vk::MemoryPropertyFlags memoryPropertyFlags,
                vk::ImageAspectFlags aspectFlags, vk::SampleCountFlagBits msaa, uint32_t mipLevels);

    [[nodiscard]] auto getImage() const noexcept -> vk::Image {
        return m_image.get();
    }

    [[nodiscard]] auto getMemory() const noexcept -> vk::DeviceMemory {
        return m_memory.get();
    }

    [[nodiscard]] auto getImageView() const noexcept -> vk::ImageView {
        return m_imageView.get();
    }

    [[nodiscard]] auto getExtent() const noexcept -> vk::Extent3D {
        return m_extent;
    }

    void resize(vk::Extent2D resolution);
    void resize(vk::Extent3D resolution);

    void transitImageLayout(ImageLayoutTransition layoutTransition);
    void transitImageLayout(vk::CommandBuffer commandBuffer, ImageLayoutTransition layoutTransition);

    void copyTo(vk::CommandBuffer commandBuffer, const ImageMemory& dstImage);
    void copyTo(vk::CommandBuffer commandBuffer, vk::Image image);
    void blitTo(vk::CommandBuffer commandBuffer, const ImageMemory& dstImage);
    void blitTo(vk::CommandBuffer commandBuffer, vk::Image dstImage, vk::Extent3D dstResolution);

private:
    vk::UniqueImage m_image;
    vk::UniqueDeviceMemory m_memory;
    vk::UniqueImageView m_imageView;

    vk::Format m_format;
    vk::ImageUsageFlags m_imageUsageFlags;
    vk::MemoryPropertyFlags m_memoryPropertyFlags;
    vk::ImageAspectFlags m_aspectFlags;
    vk::SampleCountFlagBits m_msaa;
    uint32_t m_mipLevels{ 1 };
    vk::Extent3D m_extent{};

    VulkanDevice m_device;
};

class DepthImageMemory: public ImageMemory {
public:
    DepthImageMemory(const VulkanDevice& device, vk::Extent2D resolution, vk::Format format,
                     vk::SampleCountFlagBits msaa);
};

class ColorImageMemory: public ImageMemory {
public:
    ColorImageMemory(const VulkanDevice& device, vk::Extent2D resolution, vk::Format format,
                     vk::SampleCountFlagBits msaa);
};

class Vulkan2DTexture {
public:
    Vulkan2DTexture(const VulkanDevice& device, const TextureData& texture,
                    vk::Format format = vk::Format::eR8G8B8A8Unorm);

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
    void generateMipmaps() const;

private:
    ImageMemory m_imageMemory;
    vk::UniqueSampler m_sampler;
    vk::Format m_format;

    VulkanDevice m_device;

    vk::Extent2D m_extent{};
    uint32_t m_mipLevels{};
};

}// namespace th::vulkan