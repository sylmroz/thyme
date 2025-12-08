export module th.render_system.vulkan:swapchain;

import std;

import vulkan_hpp;

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

private:
    [[nodiscard]] auto getSwapchainFrameInternal(const std::uint32_t index) const -> SwapchainFrame {
        if (index >= m_images.size()) {
            constexpr auto message = "SwapChainFrames index out of range";
            throw std::out_of_range(message);
        }
        return SwapchainFrame{ .image = m_images[index], .image_view = m_imageViews[index] };
    }

private:
    std::vector<vk::Image> m_images;
    std::vector<vk::raii::ImageView> m_imageViews;
};

export class SwapchainData {
public:
    explicit SwapchainData(const VulkanDeviceRAII& device, const SwapChainSettings& swapchain_settings,
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

private:
    vk::raii::SwapchainKHR m_swapchain;
    SwapchainFrames m_swapchain_frames;

private:
    static [[nodiscard]] auto createSwapchain(const VulkanDeviceRAII& device, SwapChainSettings swapchain_settings,
                                              vk::Extent2D swapchain_extent, vk::SurfaceKHR surface,
                                              vk::SwapchainKHR old_swapchain) -> vk::raii::SwapchainKHR;
};

export class VulkanSwapchain final {
public:
    VulkanSwapchain(const VulkanDeviceRAII& device, vk::SurfaceKHR surface, const VulkanGraphicContext& context,
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

    [[nodiscard]] auto hasResized() const -> bool;

private:
    auto recreateSwapchain() -> bool;

private:
    uint32_t m_current_image_index{ 0 };
    vk::SurfaceKHR m_surface;
    vk::Extent2D m_swapchain_extent;
    vk::Extent2D m_fallback_extent;
    VulkanGraphicContext m_context;
    const VulkanDeviceRAII& m_device;
    SwapchainData m_swapchain_data;
    VulkanCommandBuffersPool& m_command_buffers_pool;
    Logger& m_logger;

    std::vector<vk::raii::Semaphore> m_image_rendering_semaphore;
};

}// namespace th
