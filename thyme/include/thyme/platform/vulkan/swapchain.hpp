#pragma once

#include <vulkan/vulkan.hpp>

#include <thyme/platform/vulkan/utils.hpp>

namespace th::vulkan {

class SwapChain {
public:
    explicit SwapChain(const Device& device, const SwapChainSettings& swapChainSettings, const vk::Extent2D frameSize,
                       const vk::SurfaceKHR surface, const vk::SwapchainKHR oldSwapChain = {}) noexcept;

    [[nodiscard]] auto getSwapChain() -> vk::SwapchainKHR {
        return m_swapChain.get();
    }

    [[nodiscard]] auto getSwapChain() const -> vk::SwapchainKHR {
        return m_swapChain.get();
    }

private:
    vk::UniqueSwapchainKHR m_swapChain;
};

struct SwapChainFrame {
    vk::Image image;
    vk::UniqueImageView imageView;
};

class SwapChainFrames {
public:
    explicit SwapChainFrames(const vk::Device device, const SwapChain& swapChain, const vk::Format format) noexcept;

    [[nodiscard]] auto getSwapChainFrames() noexcept -> const std::vector<SwapChainFrame>& {
        return m_swapChainFrames;
    }

    [[nodiscard]] auto getSwapChainFrames() const noexcept -> const std::vector<SwapChainFrame>& {
        return m_swapChainFrames;
    }

private:
    std::vector<SwapChainFrame> m_swapChainFrames;
};

class SwapChainData {
public:
    explicit SwapChainData(const Device& device, const SwapChainSettings& swapChainSettings,
                           const vk::Extent2D swapChainExtent, const vk::SurfaceKHR surface,
                           const vk::SwapchainKHR oldSwapChain = {})
        : m_swapChain{ device, swapChainSettings, swapChainExtent, surface, oldSwapChain },
          m_swapChainFrames{ device.logicalDevice.get(), m_swapChain, swapChainSettings.surfaceFormat.format } {}

    [[nodiscard]] auto getSwapChain() noexcept -> vk::SwapchainKHR {
        return m_swapChain.getSwapChain();
    }

    [[nodiscard]] auto getSwapChain() const noexcept -> vk::SwapchainKHR {
        return m_swapChain.getSwapChain();
    }

    [[nodiscard]] auto getSwapChainFrames() noexcept -> const std::vector<SwapChainFrame>& {
        return m_swapChainFrames.getSwapChainFrames();
    }

    [[nodiscard]] auto getSwapChainFrames() const noexcept -> const std::vector<SwapChainFrame>& {
        return m_swapChainFrames.getSwapChainFrames();
    }

private:
    SwapChain m_swapChain;
    SwapChainFrames m_swapChainFrames;
};

}// namespace th::vulkan
