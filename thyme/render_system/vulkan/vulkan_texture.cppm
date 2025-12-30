export module th.render_system.vulkan:texture;

import std;

import vulkan;

import th.scene.texture_data;

import :buffer;
import :device;
import :utils;

namespace th {

export void copyImage(vk::CommandBuffer command_buffer, vk::Image src_image, vk::Extent3D src_resolution,
                      vk::Image dst_image);
export void blitImage(vk::CommandBuffer command_buffer, vk::Image src_image, vk::Extent3D src_resolution,
                      vk::Image dst_image, vk::Extent3D dst_resolution);

struct ImageMemoryImageView {
    vk::raii::Image image;
    vk::raii::DeviceMemory memory;
    vk::raii::ImageView image_view;
};

export class VulkanImageMemoryCreator {
public:
    VulkanImageMemoryCreator(vk::Format format, vk::ImageUsageFlags image_usage_flags,
                             vk::MemoryPropertyFlags memory_property_flags, vk::ImageAspectFlags aspect_flags,
                             vk::SampleCountFlagBits msaa, uint32_t mip_levels);

    [[nodiscard]] auto create(const VulkanDeviceRAII& device, vk::Extent3D resolution) const -> ImageMemoryImageView;

    [[nodiscard]] auto getMipLevels() const -> uint32_t {
        return m_mip_levels;
    }

    [[nodiscard]] auto getImageAspectFlags() const noexcept -> vk::ImageAspectFlags {
        return m_aspect_flags;
    }

private:
    vk::Format m_format;
    vk::ImageUsageFlags m_image_usage_flags;
    vk::MemoryPropertyFlags m_memory_property_flags;
    vk::ImageAspectFlags m_aspect_flags;
    vk::SampleCountFlagBits m_msaa;
    uint32_t m_mip_levels{ 1 };
};

export class VulkanImageMemory {
public:
    VulkanImageMemory(const VulkanDeviceRAII& device, vk::Extent3D resolution, VulkanImageMemoryCreator memory_creator,
                      const ImageTransition& image_transition);

    [[nodiscard]] auto getImage() const noexcept -> vk::Image {
        return *m_image_memory_image_view.image;
    }

    [[nodiscard]] auto getMemory() const noexcept -> vk::DeviceMemory {
        return *m_image_memory_image_view.memory;
    }

    [[nodiscard]] auto getImageView() const noexcept -> vk::ImageView {
        return *m_image_memory_image_view.image_view;
    }

    [[nodiscard]] auto getExtent() const noexcept -> vk::Extent3D {
        return m_extent;
    }

    void resize(const VulkanDeviceRAII& device, vk::Extent2D resolution);
    void resize(const VulkanDeviceRAII& device, vk::Extent3D resolution);

    void transitImageLayout(const VulkanDeviceRAII& device, ImageLayoutTransition layout_transition) const;
    void transitImageLayout(vk::CommandBuffer command_buffer, ImageLayoutTransition layout_transition) const;
    void transitImageLayout(const VulkanDeviceRAII& device, const ImageTransition& transition);
    void transitImageLayout(vk::CommandBuffer command_buffer, const ImageTransition& transition);

    [[nodiscard]] auto getImageMemoryBarrier(const ImageTransition& transition) -> vk::ImageMemoryBarrier2 {
        return m_image_layout_transition.getImageMemoryBarrier(transition);
    }

    void copyTo(vk::CommandBuffer command_buffer, const VulkanImageMemory& dst_image) const;
    void copyTo(vk::CommandBuffer command_buffer, vk::Image image) const;
    void blitTo(vk::CommandBuffer command_buffer, const VulkanImageMemory& dst_image) const;
    void blitTo(vk::CommandBuffer command_buffer, vk::Image dst_image, vk::Extent3D dst_resolution) const;

private:
    ImageMemoryImageView m_image_memory_image_view;
    vk::Extent3D m_extent{};

    VulkanImageMemoryCreator m_image_memory_creator;
    ImageLayoutTransitionState m_image_layout_transition;
    ImageTransition m_initial_state;
};

export class VulkanDepthImageMemory: public VulkanImageMemory {
public:
    VulkanDepthImageMemory(const VulkanDeviceRAII& device, vk::Extent2D resolution, vk::Format format,
                           vk::SampleCountFlagBits msaa);
};

export class VulkanColorImageMemory: public VulkanImageMemory {
public:
    VulkanColorImageMemory(const VulkanDeviceRAII& device, vk::Extent2D resolution, vk::Format format,
                           vk::SampleCountFlagBits msaa);
};

export class Vulkan2DTexture {
public:
    Vulkan2DTexture(const VulkanDeviceRAII& device, const TextureData& texture,
                    vk::Format format = vk::Format::eR8G8B8A8Unorm);

    [[nodiscard]] auto getImage() const noexcept -> vk::Image {
        return m_imageMemory.getImage();
    }
    [[nodiscard]] auto getImageView() const noexcept -> vk::ImageView {
        return m_imageMemory.getImageView();
    }
    [[nodiscard]] auto getSampler() const noexcept -> vk::Sampler {
        return *m_sampler;
    }
    [[nodiscard]] auto getImageMemory() const noexcept -> const VulkanImageMemory& {
        return m_imageMemory;
    }
    [[nodiscard]] auto getDescriptorImageInfo(const vk::ImageLayout image_layout) const noexcept
            -> vk::DescriptorImageInfo {
        return { *m_sampler, getImageView(), image_layout };
    }

    void setData(const VulkanDeviceRAII& device, const TextureData& texture);

private:
    void generateMipmaps(const VulkanDeviceRAII& device) const;

private:
    VulkanImageMemory m_imageMemory;
    vk::raii::Sampler m_sampler;
    vk::Format m_format;

    vk::Extent2D m_extent{};
    uint32_t m_mipLevels{};
};

}// namespace th
