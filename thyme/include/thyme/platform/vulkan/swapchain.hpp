#pragma once

#include <vulkan/vulkan.hpp>

#include <thyme/platform/vulkan/utils.hpp>

namespace th::vulkan {

struct SwapChainFrame {
    vk::Image image;
    vk::ImageView imageView;
};

class SwapChainFrames {
public:
    explicit SwapChainFrames(const vk::Device device, const vk::SwapchainKHR swapChain, const vk::Format format);

    [[nodiscard]] std::size_t getSwapChainFramesCount() const noexcept {
        return m_images.size();
    }

    [[nodiscard]] auto getSwapChainFrame(const uint32_t index) noexcept -> SwapChainFrame {
        return getSwapChainFrameInternal(index);
    }
    [[nodiscard]] auto getSwapChainFrame(const uint32_t index) const noexcept -> SwapChainFrame {
        return getSwapChainFrameInternal(index);
    }

private:
    [[nodiscard]] auto getSwapChainFrameInternal(const uint32_t index) const -> SwapChainFrame {
        if (index >= m_images.size()) {
            constexpr auto message = "SwapChainFrames index out of range";
            TH_API_LOG_ERROR(message);
            throw std::out_of_range(message);
        }
        return SwapChainFrame{
            .image = m_images[index],
            .imageView = m_imageViews[index].get(),
        };
    }

private:
    std::vector<vk::Image> m_images;
    std::vector<vk::UniqueImageView> m_imageViews;
};

class SwapChainData {
public:
    explicit SwapChainData(const Device& device, const SwapChainSettings& swapChainSettings,
                           const vk::Extent2D swapChainExtent, const vk::SurfaceKHR surface,
                           const vk::SwapchainKHR oldSwapChain = {})
        : m_swapChain{ createSwapChain(device, swapChainSettings, swapChainExtent, surface, oldSwapChain) },
          m_swapChainFrames{ device.logicalDevice.get(), m_swapChain.get(), swapChainSettings.surfaceFormat.format } {}

    [[nodiscard]] auto getSwapChain() noexcept -> vk::SwapchainKHR {
        return m_swapChain.get();
    }

    [[nodiscard]] auto getSwapChain() const noexcept -> vk::SwapchainKHR {
        return m_swapChain.get();
    }

    [[nodiscard]] std::size_t getSwapChainFramesCount() const noexcept {
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
    static [[nodiscard]] auto createSwapChain(const Device& device, const SwapChainSettings& swapChainSettings,
                                  const vk::Extent2D swapChainExtent, const vk::SurfaceKHR surface,
                                  const vk::SwapchainKHR oldSwapChain)
            ->vk::UniqueSwapchainKHR;
};

}// namespace th::vulkan
