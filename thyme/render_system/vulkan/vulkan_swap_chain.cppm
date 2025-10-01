export module th.render_system.vulkan:swap_chain;

import std;

import vulkan_hpp;

import th.core.logger;

import :device;
import :graphic_context;
import :utils;
import :command_buffers;
import :texture;

namespace th {

export struct SwapChainFrame {
    vk::Image image;
    vk::ImageView imageView;
};

export class SwapChainFrames {
public:
    explicit SwapChainFrames(vk::Device device, vk::SwapchainKHR swapChain, vk::Format format);

    [[nodiscard]] auto getSwapChainFramesCount() const noexcept -> std::size_t {
        return m_images.size();
    }

    [[nodiscard]] auto getSwapChainFrame(const std::uint32_t index) noexcept -> SwapChainFrame {
        return getSwapChainFrameInternal(index);
    }
    [[nodiscard]] auto getSwapChainFrame(const std::uint32_t index) const noexcept -> SwapChainFrame {
        return getSwapChainFrameInternal(index);
    }

private:
    [[nodiscard]] auto getSwapChainFrameInternal(const std::uint32_t index) const -> SwapChainFrame {
        if (index >= m_images.size()) {
            constexpr auto message = "SwapChainFrames index out of range";
            throw std::out_of_range(message);
        }
        return SwapChainFrame{ .image = m_images[index], .imageView = m_imageViews[index].get() };
    }

private:
    std::vector<vk::Image> m_images;
    std::vector<vk::UniqueImageView> m_imageViews;
};

export class SwapChainData {
public:
    explicit SwapChainData(const VulkanDevice& device, const SwapChainSettings& swapChainSettings,
                           const vk::Extent2D swapChainExtent, const vk::SurfaceKHR surface,
                           const vk::SwapchainKHR oldSwapChain = {})
        : m_swapChain{ createSwapChain(device, swapChainSettings, swapChainExtent, surface, oldSwapChain) },
          m_swapChainFrames{ device.logicalDevice, m_swapChain.get(), swapChainSettings.surfaceFormat.format } {}

    [[nodiscard]] auto getSwapChain() noexcept -> vk::SwapchainKHR {
        return m_swapChain.get();
    }

    [[nodiscard]] auto getSwapChain() const noexcept -> vk::SwapchainKHR {
        return m_swapChain.get();
    }

    [[nodiscard]] auto getSwapChainFramesCount() const noexcept -> std::size_t {
        return m_swapChainFrames.getSwapChainFramesCount();
    }

    [[nodiscard]] auto getSwapChainFrame(const uint32_t index) noexcept -> SwapChainFrame {
        return m_swapChainFrames.getSwapChainFrame(index);
    }

    [[nodiscard]] auto getSwapChainFrame(const uint32_t index) const noexcept -> SwapChainFrame {
        return m_swapChainFrames.getSwapChainFrame(index);
    }

private:
    vk::UniqueSwapchainKHR m_swapChain;
    SwapChainFrames m_swapChainFrames;

private:
    static [[nodiscard]] auto createSwapChain(const VulkanDevice& device, SwapChainSettings swapChainSettings,
                                              vk::Extent2D swapChainExtent, vk::SurfaceKHR surface,
                                              vk::SwapchainKHR oldSwapChain) -> vk::UniqueSwapchainKHR;
};

export class VulkanSwapChain final {
public:
    VulkanSwapChain(const VulkanDevice& device, vk::SurfaceKHR surface, const VulkanGraphicContext& context, vk::Extent2D swapChainExtent,
              VulkanCommandBuffersPool& commandPool, Logger& logger);
    void frameResized(vk::Extent2D resolution);
    auto prepareFrame() -> bool;
    void prepareRenderMode();
    void preparePresentMode();
    void submitFrame();

    void renderImage(vk::Image image);

    [[nodiscard]] auto getSwapChainExtent() const noexcept -> vk::Extent2D {
        return m_swapChainExtent;
    }

    [[nodiscard]] auto getCurrentSwapChainFrame() noexcept -> SwapChainFrame {
        return m_swapChainData.getSwapChainFrame(m_currentImageIndex);
    }

    [[nodiscard]] auto hasResized() const -> bool;

private:
    auto recreateSwapChain() -> bool;

private:
    uint32_t m_currentImageIndex{ 0 };
    vk::SurfaceKHR m_surface;
    vk::Extent2D m_swapChainExtent;
    vk::Extent2D m_fallBackExtent;
    VulkanGraphicContext m_context;
    VulkanDevice m_device;
    SwapChainData m_swapChainData;
    VulkanCommandBuffersPool& m_commandBuffersPool;
    Logger& m_logger;

    std::vector<vk::UniqueSemaphore> m_imageRenderingSemaphore;
};

}// namespace th::render_system::vulkan
