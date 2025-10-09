export module th.render_system.vulkan:texture;

import std;

import vulkan_hpp;

import th.scene.texture_data;

import :buffer;
import :device;
import :utils;

namespace th {

export void copyImage(vk::CommandBuffer command_buffer, vk::Image src_image, vk::Extent3D src_resolution,
                      vk::Image dst_image);
export void blitImage(vk::CommandBuffer command_buffer, vk::Image src_image, vk::Extent3D src_resolution,
                      vk::Image dst_image, vk::Extent3D dst_resolution);

export class VulkanImageMemory {
public:
    VulkanImageMemory(const VulkanDevice& device, vk::Extent3D resolution, vk::Format format,
                      vk::ImageUsageFlags image_usage_flags, vk::MemoryPropertyFlags memory_property_flags,
                      vk::ImageAspectFlags aspect_flags, vk::SampleCountFlagBits msaa, uint32_t mip_levels);

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

    void transitImageLayout(ImageLayoutTransition layout_transition);
    void transitImageLayout(vk::CommandBuffer command_buffer, ImageLayoutTransition layout_transition);

    void copyTo(vk::CommandBuffer command_buffer, const VulkanImageMemory& dst_image);
    void copyTo(vk::CommandBuffer command_buffer, vk::Image image);
    void blitTo(vk::CommandBuffer command_buffer, const VulkanImageMemory& dst_image);
    void blitTo(vk::CommandBuffer command_buffer, vk::Image dst_image, vk::Extent3D dst_resolution);

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

export class VulkanDepthImageMemory: public VulkanImageMemory {
public:
    VulkanDepthImageMemory(const VulkanDevice& device, vk::Extent2D resolution, vk::Format format,
                           vk::SampleCountFlagBits msaa);
};

export class VulkanColorImageMemory: public VulkanImageMemory {
public:
    VulkanColorImageMemory(const VulkanDevice& device, vk::Extent2D resolution, vk::Format format,
                           vk::SampleCountFlagBits msaa);
};

export class Vulkan2DTexture {
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
    [[nodiscard]] auto getImageMemory() const noexcept -> const VulkanImageMemory& {
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
    VulkanImageMemory m_imageMemory;
    vk::UniqueSampler m_sampler;
    vk::Format m_format;

    VulkanDevice m_device;

    vk::Extent2D m_extent{};
    uint32_t m_mipLevels{};
};

}// namespace th
