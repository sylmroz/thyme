#include <thyme/platform/vulkan/vulkan_swapchain.hpp>

namespace th::vulkan {

SwapChainFrames::SwapChainFrames(const vk::Device device, const vk::SwapchainKHR swapChain, const vk::Format format) {
    m_images = device.getSwapchainImagesKHR(swapChain);
    for (const auto image : m_images) {
        m_imageViews.emplace_back(device.createImageViewUnique(vk::ImageViewCreateInfo{
                .image = image,
                .viewType = vk::ImageViewType::e2D,
                .format = format,
                .subresourceRange = vk::ImageSubresourceRange{ .aspectMask = vk::ImageAspectFlagBits::eColor,
                                                               .baseMipLevel = 0,
                                                               .levelCount = 1,
                                                               .baseArrayLayer = 0,
                                                               .layerCount = 1 },
        }));
    }
}

auto SwapChainData::createSwapChain(const VulkanDevice& device, const SwapChainSettings swapChainSettings,
                                    const vk::Extent2D swapChainExtent, const vk::SurfaceKHR surface,
                                    const vk::SwapchainKHR oldSwapChain) -> vk::UniqueSwapchainKHR {
    const auto& [surfaceFormat, presetMode, imageCount] = swapChainSettings;
    const auto capabilities = device.physicalDevice.getSurfaceCapabilitiesKHR(surface);
    const auto swapChainCreateInfo = [&] {
        auto info = vk::SwapchainCreateInfoKHR{ .surface = surface,
                                                .minImageCount = imageCount,
                                                .imageFormat = surfaceFormat.format,
                                                .imageColorSpace = surfaceFormat.colorSpace,
                                                .imageExtent = swapChainExtent,
                                                .imageArrayLayers = 1,
                                                .imageUsage = vk::ImageUsageFlagBits::eColorAttachment
                                                              | vk::ImageUsageFlagBits::eTransferDst,
                                                .preTransform = capabilities.currentTransform,
                                                .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                                .presentMode = presetMode,
                                                .clipped = vk::True,
                                                .oldSwapchain = oldSwapChain };
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

VulkanSwapChain::VulkanSwapChain(const VulkanDevice& device, const vk::SurfaceKHR surface,
                                 const VulkanGraphicContext& context, const vk::Extent2D swapChainExtent,
                                 VulkanCommandBuffersPool* commandPool)
    : m_surface{ surface }, m_swapChainExtent{ swapChainExtent }, m_context{ context }, m_device{ device },
      m_swapChainData{ device,
                       SwapChainSettings{ .surfaceFormat = context.surfaceFormat,
                                          .presetMode = context.presentMode,
                                          .imageCount = context.imageCount },
                       swapChainExtent,
                       surface },
      m_commandBuffersPool{ commandPool } {
    std::generate_n(std::back_inserter(m_imageRenderingSemaphore), context.imageCount, [device]() {
        return device.logicalDevice.createSemaphoreUnique(vk::SemaphoreCreateInfo());
    });
}

bool VulkanSwapChain::prepareFrame() {
    auto imageAvailableSemaphore = m_device.logicalDevice.createSemaphoreUnique(vk::SemaphoreCreateInfo());
    const auto imageIndexResult = [&] {
        try {
            return m_device.logicalDevice.acquireNextImageKHR(m_swapChainData.getSwapChain(),
                                                              std::numeric_limits<uint64_t>::max(),
                                                              imageAvailableSemaphore.get());
        } catch (const vk::OutOfDateKHRError&) {
            recreateSwapChain();
        };

        return vk::ResultValue(vk::Result::eErrorOutOfDateKHR, std::numeric_limits<uint32_t>::max());
    }();
    if (imageIndexResult.result != vk::Result::eSuccess) {
        return false;
    }
    m_commandBuffersPool->waitFor(imageAvailableSemaphore);
    m_currentImageIndex = imageIndexResult.value;
    return true;
}

void VulkanSwapChain::frameResized(const vk::Extent2D resolution) {
    m_fallBackExtent = vk::Extent2D{ resolution.width, resolution.height };
}

void VulkanSwapChain::prepareRenderMode() {
    const auto commandBuffer = m_commandBuffersPool->get().getBuffer();
    setCommandBufferFrameSize(commandBuffer, m_swapChainExtent);
    transitImageLayout(commandBuffer,
                       getCurrentSwapChainFrame().image,
                       ImageLayoutTransition{ .oldLayout = vk::ImageLayout::eUndefined,
                                              .newLayout = vk::ImageLayout::eTransferDstOptimal },
                       ImagePipelineStageTransition{ .oldStage = vk::PipelineStageFlagBits::eTopOfPipe
                                                                 | vk::PipelineStageFlagBits::eColorAttachmentOutput
                                                                 | vk::PipelineStageFlagBits::eTransfer,
                                                     .newStage = vk::PipelineStageFlagBits::eColorAttachmentOutput
                                                                 | vk::PipelineStageFlagBits::eTransfer },
                       ImageAccessFlagsTransition{ .oldAccess = vk::AccessFlagBits::eMemoryWrite,
                                                   .newAccess = vk::AccessFlagBits::eMemoryWrite
                                                                | vk::AccessFlagBits::eMemoryRead },
                       vk::ImageAspectFlagBits::eColor,
                       1);
}

void VulkanSwapChain::preparePresentMode() {
    const auto commandBuffer = m_commandBuffersPool->get().getBuffer();
    transitImageLayout(commandBuffer,
                       m_swapChainData.getSwapChainFrame(m_currentImageIndex).image,
                       ImageLayoutTransition{ .oldLayout = vk::ImageLayout::eTransferDstOptimal,
                                              .newLayout = vk::ImageLayout::ePresentSrcKHR },
                       1);
}

void VulkanSwapChain::submitFrame() {
    const auto presentationQueue = m_device.getPresentationQueue();
    const auto swapChain = m_swapChainData.getSwapChain();
    const auto renderFinishedSemaphore = m_imageRenderingSemaphore[m_currentImageIndex].get();
    m_commandBuffersPool->submit(renderFinishedSemaphore);
    try {
        const auto queuePresentResult = presentationQueue.presentKHR(vk::PresentInfoKHR(vk::PresentInfoKHR{
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &renderFinishedSemaphore,
                .swapchainCount = 1,
                .pSwapchains = &swapChain,
                .pImageIndices = &m_currentImageIndex,
        }));
        if (queuePresentResult == vk::Result::eErrorOutOfDateKHR || queuePresentResult == vk::Result::eSuboptimalKHR) {
            recreateSwapChain();
        }
        if (queuePresentResult != vk::Result::eSuccess) {
            TH_API_LOG_ERROR("Failed to present rendered result!");
            throw std::runtime_error("Failed to present rendered result!");
        }
    } catch (const vk::OutOfDateKHRError&) {
        recreateSwapChain();
    }
}
void VulkanSwapChain::renderImage(const vk::Image image) {
    prepareRenderMode();
    const auto blitSize =
            vk::Extent3D{ .width = getSwapChainExtent().width, .height = getSwapChainExtent().height, .depth = 1 };
    blitImage(m_commandBuffersPool->get().getBuffer(), image, blitSize, getCurrentSwapChainFrame().image, blitSize);
    preparePresentMode();
    submitFrame();
}

bool VulkanSwapChain::hasResized() const {
    const auto caps = m_device.physicalDevice.getSurfaceCapabilitiesKHR(m_surface);
    return caps.currentExtent.width != m_swapChainExtent.width || caps.currentExtent.height != m_swapChainExtent.height;
}

bool VulkanSwapChain::recreateSwapChain() {
    m_commandBuffersPool->flush();
    m_device.logicalDevice.waitIdle();
    m_swapChainExtent = [resolution = m_fallBackExtent, surface = m_surface, physicalDevice = m_device.physicalDevice] {
        const auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        const auto [width, height] = resolution;
        const auto minImageExtent = capabilities.minImageExtent;
        const auto maxImageExtent = capabilities.maxImageExtent;
        return vk::Extent2D{ .width = std::clamp(width, minImageExtent.width, maxImageExtent.width),
                             .height = std::clamp(height, minImageExtent.height, maxImageExtent.height) };
    }();
    if (m_swapChainExtent.width == 0 || m_swapChainExtent.height == 0) {
        return false;
    }
    m_swapChainData = SwapChainData(m_device,
                                    SwapChainSettings{ .surfaceFormat = m_context.surfaceFormat,
                                                       .presetMode = m_context.presentMode,
                                                       .imageCount = m_context.imageCount },
                                    m_swapChainExtent,
                                    m_surface,
                                    m_swapChainData.getSwapChain());
    return true;
}

}// namespace th::vulkan