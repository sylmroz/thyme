#include <thyme/platform/vulkan/swapchain.hpp>

namespace th::vulkan {

SwapChainFrames::SwapChainFrames(const vk::Device device, const vk::SwapchainKHR swapChain, const vk::Format format) {
    m_images = device.getSwapchainImagesKHR(swapChain);
    for (const auto image : m_images) {
        m_imageViews.emplace_back(device.createImageViewUnique(
                vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(),
                                        image,
                                        vk::ImageViewType::e2D,
                                        format,
                                        vk::ComponentMapping(),
                                        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))));
    }
}

auto SwapChainData::createSwapChain(const VulkanDevice& device, const SwapChainSettings& swapChainSettings,
                                    const vk::Extent2D swapChainExtent, const vk::SurfaceKHR surface,
                                    const vk::SwapchainKHR oldSwapChain) -> vk::UniqueSwapchainKHR {
    const auto& [surfaceFormat, presetMode, imageCount] = swapChainSettings;
    const auto swapChainCreateInfo = [&] {
        auto info = vk::SwapchainCreateInfoKHR(vk::SwapchainCreateFlagsKHR(),
                                               surface,
                                               imageCount,
                                               surfaceFormat.format,
                                               surfaceFormat.colorSpace,
                                               swapChainExtent,
                                               1,
                                               vk::ImageUsageFlagBits::eColorAttachment);
        info.preTransform = device.swapChainSupportDetails.capabilities.currentTransform;
        info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        info.presentMode = presetMode;
        info.clipped = vk::True;
        info.oldSwapchain = oldSwapChain;
        if (device.queueFamilyIndices.graphicFamily.value() != device.queueFamilyIndices.presentFamily.value()) {
            const auto indices = std::array{ device.queueFamilyIndices.graphicFamily.value(),
                                             device.queueFamilyIndices.presentFamily.value() };
            info.imageSharingMode = vk::SharingMode::eConcurrent;
            info.setQueueFamilyIndices(indices);
        } else {
            info.imageSharingMode = vk::SharingMode::eExclusive;
        }
        return info;
    }();
    return device.logicalDevice.createSwapchainKHRUnique(swapChainCreateInfo);
}
}// namespace th::vulkan