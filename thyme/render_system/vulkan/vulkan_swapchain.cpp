module;

module th.render_system.vulkan;

namespace th {

SwapchainFrames::SwapchainFrames(const vk::raii::Device& device, const vk::raii::SwapchainKHR& swapchain,
                                 const vk::Format format) {
    m_images = swapchain.getImages();
    for (const auto image : m_images) {
        m_image_views.emplace_back(device.createImageView(vk::ImageViewCreateInfo{
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

auto SwapchainData::createSwapchain(const VulkanDeviceRAII& device, const SwapChainSettings swapchain_settings,
                                    const vk::Extent2D swapchain_extent, const vk::SurfaceKHR surface,
                                    const vk::SwapchainKHR old_swapchain) -> vk::raii::SwapchainKHR {
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
    return device.logical_device.createSwapchainKHR(swapChainCreateInfo);
}

VulkanSwapchain::VulkanSwapchain(const VulkanDeviceRAII& device, const vk::SurfaceKHR surface,
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
    std::generate_n(std::back_inserter(m_image_rendering_semaphore), context.image_count, [&device] {
        return device.logical_device.createSemaphore(vk::SemaphoreCreateInfo());
    });
    for (int i{ 0 }; i < m_swapchain_data.getSwapchainFramesCount(); ++i) {
        m_transition_states.emplace_back(
                m_swapchain_data.getSwapchainFrame(i).image, vk::ImageAspectFlagBits::eColor, 1, ImageTransition{});
    }
}

auto VulkanSwapchain::prepareFrame() -> bool {
    auto imageAvailableSemaphore = m_device.logical_device.createSemaphore(vk::SemaphoreCreateInfo());
    const auto [result, image_index] = [&] {
        try {
            return m_swapchain_data.getSwapchain().acquireNextImage(std::numeric_limits<std::uint64_t>::max(),
                                                                    imageAvailableSemaphore);
        } catch (const vk::OutOfDateKHRError&) {
            recreateSwapchain();
        };

        return vk::ResultValue(vk::Result::eErrorOutOfDateKHR, std::numeric_limits<std::uint32_t>::max());
    }();
    if (result != vk::Result::eSuccess) {
        return false;
    }
    m_command_buffers_pool.waitFor(m_device.logical_device, std::move(imageAvailableSemaphore));
    m_current_image_index = image_index;
    return true;
}

void VulkanSwapchain::frameResized(const vk::Extent2D resolution) {
    m_fallback_extent = vk::Extent2D{ resolution.width, resolution.height };
}

void VulkanSwapchain::prepareRenderMode() {
    const auto commandBuffer = m_command_buffers_pool.get().getBuffer(m_device.logical_device);
    setCommandBufferFrameSize(commandBuffer, m_swapchain_extent);

    /*transitImageLayout(commandBuffer,
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
                       1);*/
}

void VulkanSwapchain::preparePresentMode() {
    /*const auto commandBuffer = m_command_buffers_pool.get().getBuffer(m_device.logical_device);
    transitImageLayout(commandBuffer,
                       m_swapchain_data.getSwapchainFrame(m_current_image_index).image,
                       ImageLayoutTransition{ .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                              .newLayout = vk::ImageLayout::ePresentSrcKHR },
                       1,
                       vk::ImageAspectFlagBits::eColor);*/
}

void VulkanSwapchain::submitFrame() {
    // preparePresentMode();
    const auto presentationQueue = m_device.getPresentationQueue();
    const auto renderFinishedSemaphore = *m_image_rendering_semaphore[m_current_image_index];
    m_command_buffers_pool.submit(renderFinishedSemaphore);
    try {
        const auto queuePresentResult = presentationQueue.presentKHR(vk::PresentInfoKHR{
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &renderFinishedSemaphore,
                .swapchainCount = 1,
                .pSwapchains = &(*m_swapchain_data.getSwapchain()),
                .pImageIndices = &m_current_image_index,
        });
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
    blitImage(m_command_buffers_pool.get().getBuffer(m_device.logical_device),
              image,
              blitSize,
              getCurrentSwapchainFrame().image,
              blitSize);
}

void VulkanSwapchain::transitImageLayout(const vk::CommandBuffer command_buffer, const ImageTransition& transition) {
    m_transition_states[m_current_image_index].transitTo(command_buffer, transition);
}

auto VulkanSwapchain::getImageMemoryBarrier(const ImageTransition& transition) -> vk::ImageMemoryBarrier2 {
    return m_transition_states[m_current_image_index].getImageMemoryBarrier(transition);
}

auto VulkanSwapchain::hasResized() const -> bool {
    const auto caps = m_device.physical_device.getSurfaceCapabilitiesKHR(m_surface);
    return caps.currentExtent.width != m_swapchain_extent.width
           || caps.currentExtent.height != m_swapchain_extent.height;
}

auto VulkanSwapchain::recreateSwapchain() -> bool {
    m_command_buffers_pool.flush(m_device.logical_device);
    m_device.logical_device.waitIdle();
    m_swapchain_extent =
            [resolution = m_fallback_extent, surface = m_surface, physicalDevice = m_device.physical_device] {
                const auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
                if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
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
    for (int i{ 0 }; i < m_swapchain_data.getSwapchainFramesCount(); ++i) {
        m_transition_states[i] = ImageLayoutTransitionState(
                m_swapchain_data.getSwapchainFrame(i).image, vk::ImageAspectFlagBits::eColor, 1, ImageTransition{});
    }
    return true;
}

}// namespace th