export module th.render_system.vulkan:swapchain;

import std;

import vulkan;

import th.core.logger;

import :device;
import :graphic_context;
import :utils;
import :command_buffers;
import :texture;

namespace th {

export struct SwapchainFrame {
    vk::Image image;
    vk::ImageView image_view;
};

export class SwapchainFrames {
public:
    SwapchainFrames() = default;
    explicit SwapchainFrames(const vk::raii::Device& device, const vk::raii::SwapchainKHR& swapchain,
                             vk::Format format);

    [[nodiscard]] auto getSwapchainFramesCount() const noexcept -> std::size_t {
        return m_images.size();
    }

    [[nodiscard]] auto getSwapchainFrame(const std::uint32_t index) noexcept -> SwapchainFrame {
        return getSwapchainFrameInternal(index);
    }
    [[nodiscard]] auto getSwapchainFrame(const std::uint32_t index) const noexcept -> SwapchainFrame {
        return getSwapchainFrameInternal(index);
    }

    [[nodiscard]] auto getImageMemoryBarrier(uint32_t i, const ImageTransition& transition) -> vk::ImageMemoryBarrier2;

    void transitImageLayout(uint32_t i, vk::CommandBuffer command_buffer, const ImageTransition& transition);

private:
    [[nodiscard]] auto getSwapchainFrameInternal(const std::uint32_t index) const -> SwapchainFrame {
        if (index >= m_images.size()) {
            constexpr auto message = "SwapChainFrames index out of range";
            throw std::out_of_range(message);
        }
        return SwapchainFrame{ .image = m_images[index], .image_view = m_image_views[index] };
    }

private:
    std::vector<vk::Image> m_images;
    std::vector<vk::raii::ImageView> m_image_views;
    std::vector<ImageLayoutTransitionState> m_transition_states;
};

export class SwapchainData {
public:
    explicit SwapchainData(const VulkanDevice& device, const SwapChainSettings& swapchain_settings,
                           const vk::Extent2D swapchain_extent, const vk::SurfaceKHR surface,
                           const vk::SwapchainKHR old_swapchain = {})
        : m_swapchain{ createSwapchain(device, swapchain_settings, swapchain_extent, surface, old_swapchain) },
          m_swapchain_frames{ device.logical_device, m_swapchain, swapchain_settings.surfaceFormat.format } {}

    [[nodiscard]] auto getSwapchain() noexcept -> const vk::raii::SwapchainKHR& {
        return m_swapchain;
    }

    [[nodiscard]] auto getSwapchain() const noexcept -> const vk::raii::SwapchainKHR& {
        return m_swapchain;
    }

    [[nodiscard]] auto getSwapchainFramesCount() const noexcept -> std::size_t {
        return m_swapchain_frames.getSwapchainFramesCount();
    }

    [[nodiscard]] auto getSwapchainFrame(const uint32_t index) noexcept -> SwapchainFrame {
        return m_swapchain_frames.getSwapchainFrame(index);
    }

    [[nodiscard]] auto getSwapchainFrame(const uint32_t index) const noexcept -> SwapchainFrame {
        return m_swapchain_frames.getSwapchainFrame(index);
    }

    [[nodiscard]] auto getImageMemoryBarrier(uint32_t i, const ImageTransition& transition) -> vk::ImageMemoryBarrier2;

    void transitImageLayout(uint32_t i, vk::CommandBuffer command_buffer, const ImageTransition& transition);

private:
    vk::raii::SwapchainKHR m_swapchain;
    SwapchainFrames m_swapchain_frames;

private:
    static [[nodiscard]] auto createSwapchain(const VulkanDevice& device, SwapChainSettings swapchain_settings,
                                              vk::Extent2D swapchain_extent, vk::SurfaceKHR surface,
                                              vk::SwapchainKHR old_swapchain) -> vk::raii::SwapchainKHR;
};

export class VulkanSwapchain final {
public:
    VulkanSwapchain(const VulkanDevice& device, vk::SurfaceKHR surface, const VulkanGraphicContext& context,
                    vk::Extent2D swapchain_extent, VulkanCommandBuffersPool& command_pool, Logger& logger);
    void frameResized(vk::Extent2D resolution);
    auto prepareFrame() -> bool;
    void prepareRenderMode();
    void preparePresentMode();
    void submitFrame();

    void renderImage(vk::Image image);

    [[nodiscard]] auto getSwapchainExtent() const noexcept -> vk::Extent2D {
        return m_swapchain_extent;
    }

    [[nodiscard]] auto getCurrentSwapchainFrame() noexcept -> SwapchainFrame {
        return m_swapchain_data.getSwapchainFrame(m_current_image_index);
    }

    void transitImageLayout(vk::CommandBuffer command_buffer, const ImageTransition& transition);
    auto getImageMemoryBarrier(const ImageTransition& transition) -> vk::ImageMemoryBarrier2;

    [[nodiscard]] auto hasResized() const -> bool;

private:
    auto recreateSwapchain() -> bool;

private:
    uint32_t m_current_image_index{ 0 };
    vk::SurfaceKHR m_surface;
    vk::Extent2D m_swapchain_extent;
    vk::Extent2D m_fallback_extent;
    VulkanGraphicContext m_context;
    const VulkanDevice& m_device;
    SwapchainData m_swapchain_data;
    VulkanCommandBuffersPool& m_command_buffers_pool;
    Logger& m_logger;

    std::vector<vk::raii::Semaphore> m_image_rendering_semaphore;
};

export enum struct SwapChainCreationState {
    success,
    invalid_swapchain,
    invalid_size
};

export class VulkanSwapchain2: public RenderTarget {
public:
    VulkanSwapchain2(const vk::raii::PhysicalDevice& physical_device, const vk::raii::Device& device,
                     uint32_t queue_family_index, vk::SurfaceKHR surface,
                     std::function<vk::Extent2D()> get_frame_buffer_size, Logger& logger);

    auto prepareFrame(const vk::raii::PhysicalDevice& physical_device, const vk::raii::Device& device,
                      VulkanCommandBuffersPool2& command_buffers_pool) -> bool;
    void submitFrame(VulkanCommandBuffersPool2& command_buffers_pool, const vk::raii::Device& device);

    [[nodiscard]] auto recreateSwapchain(const vk::raii::PhysicalDevice& physical_device,
                                         const vk::raii::Device& device) -> SwapChainCreationState;

    void transitImageLayout(vk::CommandBuffer command_buffer, const ImageTransition& transition);

    [[nodiscard]] auto getImage() const noexcept -> vk::Image override;
    [[nodiscard]] auto getImageView() const noexcept -> vk::ImageView override;
    [[nodiscard]] auto getResolution() const noexcept -> vk::Extent2D override;
    [[nodiscard]] auto getImageMemoryBarrier(const ImageTransition& transition) noexcept
            -> vk::ImageMemoryBarrier2 override;

private:
    void createSwapchain(const vk::raii::PhysicalDevice& physical_device, const vk::raii::Device& device);
    [[nodiscard]] auto getExtent() const noexcept -> vk::Extent2D;

    uint32_t m_current_image_index{ 0 };
    vk::SurfaceKHR m_surface;
    vk::Extent2D m_swapchain_extent;
    std::function<vk::Extent2D()> m_get_frame_buffer_size;
    SwapChainSupportDetails m_swapchain_details;
    vk::Extent2D m_swapchain_frame_extent{};
    vk::raii::Queue m_presentation_queue;
    vk::raii::SwapchainKHR m_swapchain{ nullptr };
    SwapchainFrames m_swapchain_frames;
    std::vector<vk::raii::Semaphore> m_image_available_semaphore;
    std::vector<vk::raii::Semaphore> m_image_render_semaphore;
    uint32_t m_image_available_semaphore_index{ 0 };

    bool m_should_recreate_swapchain{ false };

    Logger& m_logger;
};

auto VulkanSwapchain2::getImage() const noexcept -> vk::Image {
    return m_swapchain_frames.getSwapchainFrame(m_current_image_index).image;
}

auto VulkanSwapchain2::getImageView() const noexcept -> vk::ImageView {
    return m_swapchain_frames.getSwapchainFrame(m_current_image_index).image_view;
}

auto VulkanSwapchain2::getResolution() const noexcept -> vk::Extent2D {
    return m_swapchain_extent;
}

auto VulkanSwapchain2::getImageMemoryBarrier(const ImageTransition& transition) noexcept
        -> vk::ImageMemoryBarrier2 {
    return m_swapchain_frames.getImageMemoryBarrier(m_current_image_index, transition);
}

}// namespace th
