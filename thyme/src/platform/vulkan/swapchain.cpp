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

auto SwapChainData::createSwapChain(const VulkanDevice& device, const SwapChainSettings swapChainSettings,
                                    const vk::Extent2D swapChainExtent, const vk::SurfaceKHR surface,
                                    const vk::SwapchainKHR oldSwapChain) -> vk::UniqueSwapchainKHR {
    const auto& [surfaceFormat, presetMode, imageCount] = swapChainSettings;
    const auto capabilities = device.physicalDevice.getSurfaceCapabilitiesKHR(surface);
    const auto swapChainCreateInfo = [&] {
        auto info = vk::SwapchainCreateInfoKHR(vk::SwapchainCreateFlagsKHR(),
                                               surface,
                                               imageCount,
                                               surfaceFormat.format,
                                               surfaceFormat.colorSpace,
                                               swapChainExtent,
                                               1,
                                               vk::ImageUsageFlagBits::eColorAttachment);
        info.preTransform = capabilities.currentTransform;
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

VulkanSwapChain::VulkanSwapChain(const VulkanDevice& device, const vk::SurfaceKHR surface,
                                 const VulkanGraphicContext& context, const vk::Extent2D swapChainExtent)
    : m_surface{ surface }, m_swapChainExtent{ swapChainExtent }, m_context{ context }, m_device{ device },
      m_swapChainData{ device,
                       SwapChainSettings{ .surfaceFormat = context.surfaceFormat,
                                          .presetMode = context.presentMode,
                                          .imageCount = context.imageCount },
                       swapChainExtent,
                       surface },
      m_depthImageMemory{ device, swapChainExtent, context.depthFormat, device.maxMsaaSamples },
      m_colorImageMemory{ device, swapChainExtent, context.surfaceFormat.format, device.maxMsaaSamples },
      m_frameDataList{ device.logicalDevice, context.maxFramesInFlight } {}

bool VulkanSwapChain::prepareFrame() {
    if (hasResized()) {
        recreateSwapChain();
    }
    m_currentFrameData = m_frameDataList.getNext();
    const auto logicalDevice = m_device.logicalDevice;
    if (logicalDevice.waitForFences({ m_currentFrameData.fence }, vk::True, std::numeric_limits<uint64_t>::max())
        != vk::Result::eSuccess) {
        TH_API_LOG_ERROR("Failed to wait for a complete fence");
        throw std::runtime_error("Failed to wait for a complete fence");
    }
    logicalDevice.resetFences({ m_currentFrameData.fence });

    const auto imageIndexResult = [&] {
        try {
            return logicalDevice.acquireNextImageKHR(m_swapChainData.getSwapChain(),
                                                     std::numeric_limits<uint64_t>::max(),
                                                     m_currentFrameData.imageAvailableSemaphore);
        } catch (const vk::OutOfDateKHRError&) {
            recreateSwapChain();
        };

        return vk::ResultValue(vk::Result::eErrorOutOfDateKHR, std::numeric_limits<uint32_t>::max());
    }();
    if (imageIndexResult.result != vk::Result::eSuccess) {
        return false;
    }
    m_currentImageIndex = imageIndexResult.value;
    return true;
}

void VulkanSwapChain::frameResized(const vk::Extent2D resolution) {
    m_fallBackExtent = vk::Extent2D{ resolution.width, resolution.height };
}

void VulkanSwapChain::prepareRenderMode(const vk::CommandBuffer commandBuffer) {
    setCommandBufferFrameSize(commandBuffer, m_swapChainExtent);

    constexpr auto clearColorValues = vk::ClearValue(vk::ClearColorValue(1.0f, 0.0f, 1.0f, 1.0f));
    constexpr auto depthClearValue = vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0));

    const auto& [image, imageView] = m_swapChainData.getSwapChainFrame(m_currentImageIndex);

    const auto colorAttachment = vk::RenderingAttachmentInfo(m_colorImageMemory.getImageView(),
                                                             vk::ImageLayout::eColorAttachmentOptimal,
                                                             vk::ResolveModeFlagBits::eAverage,
                                                             imageView,
                                                             vk::ImageLayout::eColorAttachmentOptimal,
                                                             vk::AttachmentLoadOp::eClear,
                                                             vk::AttachmentStoreOp::eStore,
                                                             clearColorValues);

    const auto depthAttachment = vk::RenderingAttachmentInfo(m_depthImageMemory.getImageView(),
                                                             vk::ImageLayout::eDepthAttachmentOptimal,
                                                             vk::ResolveModeFlagBits::eNone,
                                                             {},
                                                             {},
                                                             vk::AttachmentLoadOp::eClear,
                                                             vk::AttachmentStoreOp::eDontCare,
                                                             depthClearValue);

    const auto renderingInfo = vk::RenderingInfo(vk::RenderingFlagBits{},
                                                 vk::Rect2D(vk::Offset2D(0, 0), m_swapChainExtent),
                                                 1,
                                                 0,
                                                 { colorAttachment },
                                                 &depthAttachment);
    transitImageLayout(commandBuffer, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, 1);
    transitDepthImageLayout(commandBuffer);

    commandBuffer.beginRendering(renderingInfo);
}

void VulkanSwapChain::preparePresentMode(const vk::CommandBuffer commandBuffer) {
    commandBuffer.endRendering();
    transitImageLayout(commandBuffer,
                       m_swapChainData.getSwapChainFrame(m_currentImageIndex).image,
                       vk::ImageLayout::eColorAttachmentOptimal,
                       vk::ImageLayout::ePresentSrcKHR,
                       1);
}

void VulkanSwapChain::renderGraphic(vk::CommandBuffer commandBuffer) {
    constexpr vk::PipelineStageFlags f = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const auto submitInfo = vk::SubmitInfo({ m_currentFrameData.imageAvailableSemaphore },
                                           { f },
                                           { commandBuffer },
                                           { m_currentFrameData.renderFinishedSemaphore });
    const auto& graphicQueue = m_device.getGraphicQueue();
    graphicQueue.submit(submitInfo, m_currentFrameData.fence);
}

void VulkanSwapChain::submitFrame() {
    const auto presentationQueue = m_device.getPresentationQueue();
    const auto swapChain = m_swapChainData.getSwapChain();

    try {
        const auto queuePresentResult = presentationQueue.presentKHR(vk::PresentInfoKHR(
                { m_currentFrameData.renderFinishedSemaphore }, { swapChain }, { m_currentImageIndex }));
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
bool VulkanSwapChain::hasResized() const {
    const auto caps = m_device.physicalDevice.getSurfaceCapabilitiesKHR(m_surface);
    return caps.currentExtent.width != m_swapChainExtent.width || caps.currentExtent.height != m_swapChainExtent.height;
}

void VulkanSwapChain::recreateSwapChain() {
    m_device.logicalDevice.waitIdle();
    m_swapChainExtent = [resolution = m_fallBackExtent, surface = m_surface, physicalDevice = m_device.physicalDevice] {
        const auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        const auto [width, height] = resolution;
        const auto minImageExtent = capabilities.minImageExtent;
        const auto maxImageExtent = capabilities.maxImageExtent;
        return vk::Extent2D{ std::clamp(width, minImageExtent.width, maxImageExtent.width),
                             std::clamp(height, minImageExtent.height, maxImageExtent.height) };
    }();
    m_swapChainData = SwapChainData(m_device,
                                    SwapChainSettings{ .surfaceFormat = m_context.surfaceFormat,
                                                       .presetMode = m_context.presentMode,
                                                       .imageCount = m_context.imageCount },
                                    m_swapChainExtent,
                                    m_surface,
                                    m_swapChainData.getSwapChain());
    m_colorImageMemory.resize(m_swapChainExtent);
    m_depthImageMemory.resize(m_swapChainExtent);
}

}// namespace th::vulkan