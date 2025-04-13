#include <thyme/platform/vulkan/swapchain.hpp>

#include <ranges>

namespace th::vulkan {

SwapChain::SwapChain(const Device& device, const SwapChainSettings& swapChainSettings, const vk::Extent2D frameSize,
                     const vk::SurfaceKHR surface, const vk::SwapchainKHR oldSwapChain) noexcept {
    const auto& [surfaceFormat, presetMode, imageCount] = swapChainSettings;
    [[maybe_unused]] const auto& [physicalDevice,
                                  logicalDevice,
                                  queueFamilyIndices,
                                  swapChainSupportDetails,
                                  maxMsaaSamples] = device;
    const auto swapChainCreateInfo = [&] {
        auto info = vk::SwapchainCreateInfoKHR(vk::SwapchainCreateFlagsKHR(),
                                               surface,
                                               imageCount,
                                               surfaceFormat.format,
                                               surfaceFormat.colorSpace,
                                               frameSize,
                                               1,
                                               vk::ImageUsageFlagBits::eColorAttachment);
        info.preTransform = swapChainSupportDetails.capabilities.currentTransform;
        info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        info.presentMode = presetMode;
        info.clipped = vk::True;
        info.oldSwapchain = oldSwapChain;
        if (queueFamilyIndices.graphicFamily.value() != queueFamilyIndices.presentFamily.value()) {
            const auto indices =
                    std::array{ queueFamilyIndices.graphicFamily.value(), queueFamilyIndices.presentFamily.value() };
            info.imageSharingMode = vk::SharingMode::eConcurrent;
            info.setQueueFamilyIndices(indices);
        } else {
            info.imageSharingMode = vk::SharingMode::eExclusive;
        }
        return info;
    }();
    m_swapChain = logicalDevice->createSwapchainKHRUnique(swapChainCreateInfo);
}

SwapChainFrames::SwapChainFrames(const vk::Device device, const SwapChain& swapChain, const vk::Format format) noexcept {
    m_swapChainFrames =  device.getSwapchainImagesKHR(swapChain.getSwapChain())
                     | std::views::transform([&](vk::Image image) -> SwapChainFrame {
                           auto imageView = device.createImageViewUnique(vk::ImageViewCreateInfo(
                                   vk::ImageViewCreateFlags(),
                                   image,
                                   vk::ImageViewType::e2D,
                                   format,
                                   vk::ComponentMapping(),
                                   vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
                           return SwapChainFrame{ std::move(image), std::move(imageView) };
                       })
                     | std::ranges::to<decltype(m_swapChainFrames)>();
}
}// namespace th::vulkan