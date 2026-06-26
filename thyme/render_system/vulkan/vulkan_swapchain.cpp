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