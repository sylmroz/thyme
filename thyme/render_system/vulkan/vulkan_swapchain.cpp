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
        m_transition_states.emplace_back(image, vk::ImageAspectFlagBits::eColor, 1, ImageTransition{});
    }
}

auto SwapchainFrames::getImageMemoryBarrier(const uint32_t i, const ImageTransition& transition)
        -> vk::ImageMemoryBarrier2 {
    return m_transition_states[i].getImageMemoryBarrier(transition);
}

void SwapchainFrames::transitImageLayout(const uint32_t i, const vk::CommandBuffer command_buffer,
                                         const ImageTransition& transition) {
    return m_transition_states[i].transitTo(command_buffer, transition);
}

auto SwapchainData::getImageMemoryBarrier(const uint32_t i, const ImageTransition& transition)
        -> vk::ImageMemoryBarrier2 {
    return m_swapchain_frames.getImageMemoryBarrier(i, transition);
}

void SwapchainData::transitImageLayout(const uint32_t i, const vk::CommandBuffer command_buffer,
                                       const ImageTransition& transition) {
    m_swapchain_frames.transitImageLayout(i, command_buffer, transition);
}

auto SwapchainData::createSwapchain(const VulkanDevice& device, const SwapChainSettings swapchain_settings,
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
    std::generate_n(std::back_inserter(m_image_rendering_semaphore), context.image_count, [&device] {
        return device.logical_device.createSemaphore(vk::SemaphoreCreateInfo());
    });
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
    m_swapchain_data.transitImageLayout(m_current_image_index, command_buffer, transition);
}

auto VulkanSwapchain::getImageMemoryBarrier(const ImageTransition& transition) -> vk::ImageMemoryBarrier2 {
    return m_swapchain_data.getImageMemoryBarrier(m_current_image_index, transition);
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
    return true;
}

auto createSwapChain(const vk::raii::PhysicalDevice& physical_device, const vk::raii::Device& device,
                     const vk::SurfaceKHR surface, SwapChainSettings swapchain_settings, vk::Extent2D swapchain_extent,
                     vk::SwapchainKHR old_swapchain = {}) -> vk::raii::SwapchainKHR {
    const auto& [surfaceFormat, presetMode, imageCount] = swapchain_settings;
    const auto capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);

    const auto swap_chain_create_info =
            vk::SwapchainCreateInfoKHR{ .surface = surface,
                                        .minImageCount = imageCount,
                                        .imageFormat = surfaceFormat.format,
                                        .imageColorSpace = surfaceFormat.colorSpace,
                                        .imageExtent = swapchain_extent,
                                        .imageArrayLayers = 1,
                                        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment
                                                      | vk::ImageUsageFlagBits::eTransferDst,
                                        .imageSharingMode = vk::SharingMode::eExclusive,
                                        .preTransform = capabilities.currentTransform,
                                        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                        .presentMode = presetMode,
                                        .clipped = vk::True,
                                        .oldSwapchain = old_swapchain };

    return device.createSwapchainKHR(swap_chain_create_info);
}

VulkanSwapchain2::VulkanSwapchain2(const vk::raii::PhysicalDevice& physical_device, const vk::raii::Device& device,
                                   const uint32_t queue_family_index, const vk::SurfaceKHR surface,
                                   std::function<vk::Extent2D()> get_frame_buffer_size, Logger& logger)
    : m_surface(surface), m_get_frame_buffer_size(std::move(get_frame_buffer_size)),
      m_swapchain_details(physical_device, surface), m_swapchain_frame_extent(getExtent()),
      m_presentation_queue(device.getQueue(queue_family_index, 0)), m_logger(logger) {
    createSwapchain(physical_device, device);
}

auto VulkanSwapchain2::recreateSwapchain(const vk::raii::PhysicalDevice& physical_device,
                                         const vk::raii::Device& device) -> SwapChainCreationState {
    device.waitIdle();// how to avoid this??
    m_swapchain_details = SwapChainSupportDetails(physical_device, m_surface);
    if (!m_swapchain_details.isValid()) {
        return SwapChainCreationState::invalid_swapchain;
    }

    m_swapchain_frame_extent = getExtent();
    if (m_swapchain_frame_extent.width == 0 || m_swapchain_frame_extent.height == 0) {
        return SwapChainCreationState::invalid_size;
    }

    createSwapchain(physical_device, device);

    return SwapChainCreationState::success;
}
void VulkanSwapchain2::transitImageLayout(const vk::CommandBuffer command_buffer, const ImageTransition& transition) {
    m_swapchain_frames.transitImageLayout(m_current_image_index, command_buffer, transition);
}

void VulkanSwapchain2::createSwapchain(const vk::raii::PhysicalDevice& physical_device,
                                       const vk::raii::Device& device) {
    const auto best_formats = m_swapchain_details.getBestSwapChainSettings();
    m_swapchain_format = best_formats.surfaceFormat.format;
    m_swapchain =
            createSwapChain(physical_device, device, m_surface, best_formats, m_swapchain_frame_extent, m_swapchain);
    m_swapchain_frames = SwapchainFrames(device, m_swapchain, best_formats.surfaceFormat.format);
    m_image_available_semaphores.clear();
    m_image_render_semaphores.clear();
    m_image_available_semaphore_index = 0;
    for (size_t frame_index{ 0 }; frame_index < m_swapchain_frames.getSwapchainFramesCount(); ++frame_index) {
        m_image_available_semaphores.emplace_back(
                vk::raii::Semaphore{ device, vk::SemaphoreCreateInfo{ .flags = vk::SemaphoreCreateFlagBits{} } });
        m_image_render_semaphores.emplace_back(
                vk::raii::Semaphore{ device, vk::SemaphoreCreateInfo{ .flags = vk::SemaphoreCreateFlagBits{} } });
    }
    m_should_recreate_swapchain = false;
}

auto VulkanSwapchain2::getExtent() const noexcept -> vk::Extent2D {
    const auto [width, height] = m_get_frame_buffer_size();
    return m_swapchain_details.getSwapExtent(glm::uvec2{ width, height });
}

auto VulkanSwapchain2::prepareFrame(const vk::raii::PhysicalDevice& physical_device, const vk::raii::Device& device)
        -> std::optional<SwapChainFrameSemaphores> {
    if (m_should_recreate_swapchain) {
        const auto result = recreateSwapchain(physical_device, device);
        if (result == SwapChainCreationState::invalid_swapchain) {
            m_logger.critical("invalid swapchain creation state");
            throw std::runtime_error("Invalid swapchain creation state");
        }
        if (result == SwapChainCreationState::invalid_size) {
            return std::nullopt;
        }
    }
    try {
        const auto current_semaphore_index = m_image_available_semaphore_index;
        m_image_available_semaphore_index =
                (m_image_available_semaphore_index + 1) % m_image_available_semaphores.size();

        [[maybe_unused]] const auto result = m_swapchain.acquireNextImage(
                std::numeric_limits<uint64_t>::max(), *m_image_available_semaphores[current_semaphore_index]);
        m_current_image_index = result.value;
        return SwapChainFrameSemaphores{
            .image_available_semaphore = m_image_available_semaphores[current_semaphore_index],
            .image_rendering_semaphore = m_image_render_semaphores[m_current_image_index],
        };
    } catch (const vk::OutOfDateKHRError&) {
        m_should_recreate_swapchain = true;
    }
    return std::nullopt;
}

void VulkanSwapchain2::submitFrame() {
    const auto render_semaphore = *m_image_render_semaphores[m_current_image_index];

    const auto queuePresentResult = m_presentation_queue.presentKHR(vk::PresentInfoKHR{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &(*m_swapchain),
            .pImageIndices = &m_current_image_index,
    });
    if (queuePresentResult == vk::Result::eErrorOutOfDateKHR || queuePresentResult == vk::Result::eSuboptimalKHR) {
        m_should_recreate_swapchain = true;
    }
}

}// namespace th