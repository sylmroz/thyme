module;

module th.render_system.vulkan;

namespace th {

SwapchainFrames::SwapchainFrames(const vk::Device device, const vk::SwapchainKHR swapchain, const vk::Format format) {
    m_images = device.getSwapchainImagesKHR(swapchain);
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

auto SwapchainData::createSwapchain(const VulkanDevice& device, const SwapChainSettings swapchain_settings,
                                    const vk::Extent2D swapchain_extent, const vk::SurfaceKHR surface,
                                    const vk::SwapchainKHR old_swapchain) -> vk::UniqueSwapchainKHR {
    const auto& [surfaceFormat, presetMode, imageCount] = swapchain_settings;
    const auto capabilities = device.physical_device.getSurfaceCapabilitiesKHR(surface);
    const auto swapChainCreateInfo = [&] {
        auto info = vk::SwapchainCreateInfoKHR{ .surface = surface,
                                                .minImageCount = imageCount,
                                                .imageFormat = surfaceFormat.format,
                                                .imageColorSpace = surfaceFormat.colorSpace,
                                                .imageExtent = swapchain_extent,
                                                .imageArrayLayers = 1,
                                                .imageUsage = vk::ImageUsageFlagBits::eColorAttachment
                                                              | vk::ImageUsageFlagBits::eTransferDst,
                                                .preTransform = capabilities.currentTransform,
                                                .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                                .presentMode = presetMode,
                                                .clipped = vk::True,
                                                .oldSwapchain = old_swapchain };
        if (device.queue_family_indices.graphic_family.value() != device.queue_family_indices.present_family.value()) {
            const auto indices = std::array{ device.queue_family_indices.graphic_family.value(),
                                             device.queue_family_indices.present_family.value() };
            info.imageSharingMode = vk::SharingMode::eConcurrent;
            info.setQueueFamilyIndices(indices);
        } else {
            info.imageSharingMode = vk::SharingMode::eExclusive;
        }
        return info;
    }();
    return device.logical_device.createSwapchainKHRUnique(swapChainCreateInfo);
}

VulkanSwapchain::VulkanSwapchain(const VulkanDevice& device, const vk::SurfaceKHR surface,
                                 const VulkanGraphicContext& context, const vk::Extent2D swapchain_extent,
                                 VulkanCommandBuffersPool& command_pool, Logger& logger)
    : m_surface{ surface }, m_swapchain_extent{ swapchain_extent }, m_context{ context }, m_device{ device },
      m_swapchain_data{ device,
                       SwapChainSettings{ .surfaceFormat = context.surface_format,
                                          .presetMode = context.present_mode,
                                          .imageCount = context.image_count },
                       swapchain_extent,
                       surface },
      m_command_buffers_pool{ command_pool }, m_logger{ logger } {
    std::generate_n(std::back_inserter(m_image_rendering_semaphore), context.image_count, [device] {
        return device.logical_device.createSemaphoreUnique(vk::SemaphoreCreateInfo());
    });
}

auto VulkanSwapchain::prepareFrame() -> bool {
    auto imageAvailableSemaphore = m_device.logical_device.createSemaphoreUnique(vk::SemaphoreCreateInfo());
    const auto imageIndexResult = [&] {
        try {
            return m_device.logical_device.acquireNextImageKHR(m_swapchain_data.getSwapchain(),
                                                              std::numeric_limits<uint64_t>::max(),
                                                              imageAvailableSemaphore.get());
        } catch (const vk::OutOfDateKHRError&) {
            recreateSwapchain();
        };

        return vk::ResultValue(vk::Result::eErrorOutOfDateKHR, std::numeric_limits<uint32_t>::max());
    }();
    if (imageIndexResult.result != vk::Result::eSuccess) {
        return false;
    }
    m_command_buffers_pool.waitFor(imageAvailableSemaphore);
    m_current_image_index = imageIndexResult.value;
    return true;
}

void VulkanSwapchain::frameResized(const vk::Extent2D resolution) {
    m_fallback_extent = vk::Extent2D{ resolution.width, resolution.height };
}

void VulkanSwapchain::prepareRenderMode() {
    const auto commandBuffer = m_command_buffers_pool.get().getBuffer();
    setCommandBufferFrameSize(commandBuffer, m_swapchain_extent);
    transitImageLayout(commandBuffer,
                       getCurrentSwapchainFrame().image,
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

void VulkanSwapchain::preparePresentMode() {
    const auto commandBuffer = m_command_buffers_pool.get().getBuffer();
    transitImageLayout(commandBuffer,
                       m_swapchain_data.getSwapchainFrame(m_current_image_index).image,
                       ImageLayoutTransition{ .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                              .newLayout = vk::ImageLayout::ePresentSrcKHR },
                       1);
}

void VulkanSwapchain::submitFrame() {
    preparePresentMode();
    const auto presentationQueue = m_device.getPresentationQueue();
    const auto swapChain = m_swapchain_data.getSwapchain();
    const auto renderFinishedSemaphore = m_image_rendering_semaphore[m_current_image_index].get();
    m_command_buffers_pool.submit(renderFinishedSemaphore);
    try {
        const auto queuePresentResult = presentationQueue.presentKHR(vk::PresentInfoKHR(vk::PresentInfoKHR{
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &renderFinishedSemaphore,
                .swapchainCount = 1,
                .pSwapchains = &swapChain,
                .pImageIndices = &m_current_image_index,
        }));
        if (queuePresentResult == vk::Result::eErrorOutOfDateKHR || queuePresentResult == vk::Result::eSuboptimalKHR) {
            recreateSwapchain();
        }
        if (queuePresentResult != vk::Result::eSuccess) {
            m_logger.error("Failed to present rendered result!");
            throw std::runtime_error("Failed to present rendered result!");
        }
    } catch (const vk::OutOfDateKHRError&) {
        recreateSwapchain();
    }
}
void VulkanSwapchain::renderImage(const vk::Image image) {
    prepareRenderMode();
    const auto blitSize =
            vk::Extent3D{ .width = getSwapchainExtent().width, .height = getSwapchainExtent().height, .depth = 1 };
    blitImage(m_command_buffers_pool.get().getBuffer(), image, blitSize, getCurrentSwapchainFrame().image, blitSize);
}

auto VulkanSwapchain::hasResized() const -> bool {
    const auto caps = m_device.physical_device.getSurfaceCapabilitiesKHR(m_surface);
    return caps.currentExtent.width != m_swapchain_extent.width || caps.currentExtent.height != m_swapchain_extent.height;
}

auto VulkanSwapchain::recreateSwapchain() -> bool {
    m_command_buffers_pool.flush();
    m_device.logical_device.waitIdle();
    m_swapchain_extent = [resolution = m_fallback_extent, surface = m_surface, physicalDevice = m_device.physical_device] {
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
    if (m_swapchain_extent.width == 0 || m_swapchain_extent.height == 0) {
        return false;
    }
    m_swapchain_data = SwapchainData(m_device,
                                    SwapChainSettings{ .surfaceFormat = m_context.surface_format,
                                                       .presetMode = m_context.present_mode,
                                                       .imageCount = m_context.image_count },
                                    m_swapchain_extent,
                                    m_surface,
                                    m_swapchain_data.getSwapchain());
    return true;
}

}// namespace th