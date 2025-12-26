export module th.render_system.vulkan:utils;

import std;

import glm;
import vulkan;

import th.scene.model;

export namespace th {
struct QueueFamilyIndices {
    explicit QueueFamilyIndices(vk::PhysicalDevice device, std::optional<vk::SurfaceKHR> surface);

    std::optional<std::uint32_t> graphic_family;
    std::optional<std::uint32_t> present_family;

    [[nodiscard]] constexpr auto isCompleted() const noexcept -> bool {
        return graphic_family.has_value() && (m_requested_surface_support ? present_family.has_value() : true);
    }

private:
    bool m_requested_surface_support;
};

struct SwapChainSettings {
    vk::SurfaceFormatKHR surfaceFormat;
    vk::PresentModeKHR presetMode;
    std::uint32_t imageCount;
};

class SwapChainSupportDetails {
public:
    explicit SwapChainSupportDetails(const vk::PhysicalDevice& device, vk::SurfaceKHR surface);

    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;

    [[nodiscard]] auto isValid() const noexcept -> bool {
        return !formats.empty() && !presentModes.empty();
    }

    [[nodiscard]] auto getBestSurfaceFormat() const noexcept -> vk::SurfaceFormatKHR {
        const auto suitable_format = std::ranges::find_if(formats, [](const vk::SurfaceFormatKHR& format) {
            return format.format == vk::Format::eB8G8R8A8Unorm
                   && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
        if (suitable_format != formats.end()) {
            return *suitable_format;
        }
        return formats[0];
    }

    [[nodiscard]] auto getBestPresetMode() const noexcept -> vk::PresentModeKHR {
        const auto suitable_preset = std::ranges::find_if(presentModes, [](const vk::PresentModeKHR presentMode) {
            return presentMode == vk::PresentModeKHR::eMailbox;
        });

        if (suitable_preset != presentModes.end()) {
            return *suitable_preset;
        }
        return vk::PresentModeKHR::eFifo;
    }

    [[nodiscard]] inline auto getImageCount() const noexcept -> std::uint32_t {
        const auto swapChainImageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && swapChainImageCount > capabilities.maxImageCount) {
            return capabilities.maxImageCount;
        }
        return swapChainImageCount;
    }

    [[nodiscard]] inline auto getSwapExtent(const glm::uvec2& fallbackResolution) const noexcept -> vk::Extent2D {
        if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        const auto minImageExtent = capabilities.minImageExtent;
        const auto maxImageExtent = capabilities.maxImageExtent;
        return vk::Extent2D{ std::clamp(fallbackResolution.x, minImageExtent.width, maxImageExtent.width),
                             std::clamp(fallbackResolution.y, minImageExtent.height, maxImageExtent.height) };
    }

    [[nodiscard]] auto getBestSwapChainSettings() const noexcept -> SwapChainSettings {
        return SwapChainSettings{ .surfaceFormat = getBestSurfaceFormat(),
                                  .presetMode = getBestPresetMode(),
                                  .imageCount = getImageCount() };
    }
};

[[nodiscard]] inline auto createDescriptorPool(const vk::Device device,
                                               const std::vector<vk::DescriptorPoolSize>& descriptor_sizes)
        -> vk::UniqueDescriptorPool {
    const auto maxSet = std::accumulate(std::begin(descriptor_sizes),
                                        std::end(descriptor_sizes),
                                        uint32_t{ 0 },
                                        [](const uint32_t sum, const vk::DescriptorPoolSize& descriptorPoolSize) {
                                            return sum + descriptorPoolSize.descriptorCount;
                                        });


    return device.createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = maxSet,
            .poolSizeCount = static_cast<uint32_t>(descriptor_sizes.size()),
            .pPoolSizes = descriptor_sizes.data(),
    });
}

[[nodiscard]] inline auto createDescriptorPool(const vk::raii::Device& device,
                                               const std::span<const vk::DescriptorPoolSize> descriptor_sizes)
        -> vk::raii::DescriptorPool {
    const auto max_set = std::accumulate(std::begin(descriptor_sizes),
                                         std::end(descriptor_sizes),
                                         uint32_t{ 0 },
                                         [](const uint32_t sum, const vk::DescriptorPoolSize& descriptorPoolSize) {
                                             return sum + descriptorPoolSize.descriptorCount;
                                         });


    return device.createDescriptorPool(vk::DescriptorPoolCreateInfo{
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = max_set,
            .poolSizeCount = static_cast<uint32_t>(descriptor_sizes.size()),
            .pPoolSizes = descriptor_sizes.data(),
    });
}

template <typename F, typename... Args>
concept InvocableCommandWithCommandBuffer = requires(F f, vk::CommandBuffer commandBuffer, Args... args) {
    { f(commandBuffer, args...) } -> std::same_as<void>;
};

template <typename F, typename... Args>
    requires(InvocableCommandWithCommandBuffer<F, Args...>)
void singleTimeCommand(const vk::CommandBuffer commandBuffer, const vk::Queue graphicQueue, F fun, Args... args) {
    commandBuffer.begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    fun(commandBuffer, args...);
    commandBuffer.end();
    graphicQueue.submit(
            vk::SubmitInfo{
                    .commandBufferCount = 1u,
                    .pCommandBuffers = &commandBuffer,
            },
            nullptr);
    graphicQueue.waitIdle();
}

template <typename F, typename... Args>
    requires(InvocableCommandWithCommandBuffer<F, Args...>)
void singleTimeCommand(const vk::Device device, const vk::CommandPool commandPool, const vk::Queue graphicQueue, F fun,
                       Args... args) {
    const auto commandBuffer =
            std::move(device.allocateCommandBuffersUnique(
                                    vk::CommandBufferAllocateInfo{ .commandPool = commandPool,
                                                                   .level = vk::CommandBufferLevel::ePrimary,
                                                                   .commandBufferCount = 1u })
                              .front());
    singleTimeCommand(commandBuffer.get(), graphicQueue, fun, args...);
}

constexpr auto getBindingDescription() -> vk::VertexInputBindingDescription {
    return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
}

constexpr auto getAttributeDescriptions() -> std::array<vk::VertexInputAttributeDescription, 3> {
    return std::array{
        vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
        vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
        vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, tex_coord)),
    };
}


[[nodiscard]] inline auto findMemoryType(const vk::PhysicalDevice device, const uint32_t typeFilter,
                                         const vk::MemoryPropertyFlags properties) -> uint32_t {
    const auto& mem_properties = device.getMemoryProperties();
    for (uint32_t i{ 0 }; i < mem_properties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

inline void copyBuffer(const vk::Device device, const vk::CommandPool command_pool, const vk::Queue graphic_queue,
                       const vk::Buffer src_buffer, const vk::Buffer dst_buffer, const size_t size) {
    singleTimeCommand(device, command_pool, graphic_queue, [&](const vk::CommandBuffer& command_buffer) -> void {
        command_buffer.copyBuffer(src_buffer, dst_buffer, { vk::BufferCopy(0, 0, size) });
    });
}

struct ImageLayoutTransition {
    vk::ImageLayout oldLayout;
    vk::ImageLayout newLayout;
};

struct ImagePipelineStageTransition {
    vk::PipelineStageFlags oldStage;
    vk::PipelineStageFlags newStage;
};

struct ImageAccessFlagsTransition {
    vk::AccessFlags oldAccess;
    vk::AccessFlags newAccess;
};

struct ImageTransition {
    vk::ImageLayout layout{ vk::ImageLayout::eUndefined };
    vk::PipelineStageFlags2 pipeline_stage{ vk::PipelineStageFlagBits2::eAllCommands };
    vk::AccessFlags2 access_flag_bits{ vk::AccessFlagBits2::eNone };
    uint32_t queue_family_index{ vk::QueueFamilyIgnored };
};

class ImageLayoutTransitionState {
public:
    explicit ImageLayoutTransitionState(vk::Image image, vk::ImageAspectFlags aspect_flags, uint32_t mip_levels,
                                        ImageTransition initial_state);

    void transitTo(vk::CommandBuffer command_buffer, const ImageTransition& new_state);

private:
    vk::Image m_image;
    vk::ImageAspectFlags m_aspect_flags;
    uint32_t m_mip_levels;
    ImageTransition m_image_transition;
};

void transitImageLayout(vk::CommandBuffer command_buffer, vk::Image image, ImageLayoutTransition layout_transition,
                        uint32_t mip_levels);

void transitImageLayout(vk::CommandBuffer command_buffer, vk::Image image, ImageLayoutTransition layout_transition,
                        ImagePipelineStageTransition stage_transition,
                        ImageAccessFlagsTransition access_flags_transition, vk::ImageAspectFlags aspect_flags,
                        uint32_t mip_levels);

inline void copyBufferToImage(vk::Device device, const vk::CommandPool command_pool, const vk::Queue graphic_queue,
                              const vk::Buffer buffer, const vk::Image image, const glm::uvec2& resolution) {
    singleTimeCommand(device, command_pool, graphic_queue, [&](const vk::CommandBuffer cb) -> void {
        const auto region = vk::BufferImageCopy(0,
                                                0,
                                                0,
                                                vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                                                vk::Offset3D(0, 0, 0),
                                                vk::Extent3D(resolution.x, resolution.y, 1));
        cb.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
    });
}

[[nodiscard]] auto findSupportedImageFormat(vk::PhysicalDevice device,
                                            std::span<const vk::Format> formats,
                                            vk::ImageTiling image_tiling,
                                            vk::FormatFeatureFlags features) -> vk::Format;

[[nodiscard]] inline auto findDepthFormat(const vk::PhysicalDevice device) -> vk::Format {
    constexpr auto formats =
            std::array{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint };
    return findSupportedImageFormat(device,
                                    std::span(formats.data(), formats.size()),
                                    vk::ImageTiling::eOptimal,
                                    vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

[[nodiscard]] inline auto hasStencilFormat(const vk::Format format) noexcept -> bool {
    return format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD32SfloatS8Uint;
}

void setCommandBufferFrameSize(vk::CommandBuffer command_buffer, vk::Extent2D frame_size);

QueueFamilyIndices::QueueFamilyIndices(const vk::PhysicalDevice device, const std::optional<vk::SurfaceKHR> surface)
    : m_requested_surface_support{ surface.has_value() } {
    const auto& queue_families = device.getQueueFamilyProperties2();
    for (uint32_t i{ 0 }; i < queue_families.size(); i++) {
        const auto& queue_family = queue_families[i];
        const auto& queue_family_properties = queue_family.queueFamilyProperties;
        if (queue_family_properties.queueCount <= 0) {
            continue;
        }
        if (queue_family_properties.queueFlags & vk::QueueFlagBits::eGraphics) {
            graphic_family = i;
        }
        if (queue_family_properties.queueFlags & vk::QueueFlagBits::eCompute) {}
        if (queue_family_properties.queueFlags & vk::QueueFlagBits::eTransfer) {}
        if (surface.has_value() ? device.getSurfaceSupportKHR(i, surface.value()) : true) {
            present_family = i;
        }
        if (isCompleted()) {
            break;
        }
    }
}

SwapChainSupportDetails::SwapChainSupportDetails(const vk::PhysicalDevice& device, const vk::SurfaceKHR surface) {
    capabilities = device.getSurfaceCapabilitiesKHR(surface);
    formats = device.getSurfaceFormatsKHR(surface);
    presentModes = device.getSurfacePresentModesKHR(surface);
}

ImageLayoutTransitionState::ImageLayoutTransitionState(const vk::Image image, const vk::ImageAspectFlags aspect_flags,
                                                       const uint32_t mip_levels, ImageTransition initial_state)
    : m_image{ image }, m_aspect_flags{ aspect_flags }, m_mip_levels{ mip_levels },
      m_image_transition{ std::move(initial_state) } {}

void ImageLayoutTransitionState::transitTo(const vk::CommandBuffer command_buffer, const ImageTransition& new_state) {
    const auto image_memory_barrier =
            vk::ImageMemoryBarrier2{ .srcStageMask = m_image_transition.pipeline_stage,
                                     .srcAccessMask = m_image_transition.access_flag_bits,
                                     .dstStageMask = new_state.pipeline_stage,
                                     .dstAccessMask = new_state.access_flag_bits,
                                     .oldLayout = m_image_transition.layout,
                                     .newLayout = new_state.layout,
                                     .srcQueueFamilyIndex = m_image_transition.queue_family_index,
                                     .dstQueueFamilyIndex = new_state.queue_family_index,
                                     .image = m_image,
                                     .subresourceRange = vk::ImageSubresourceRange{
                                             .aspectMask = m_aspect_flags,
                                             .baseMipLevel = 0u,
                                             .levelCount = m_mip_levels,
                                             .baseArrayLayer = 0u,
                                             .layerCount = 1u,
                                     } };
    m_image_transition = new_state;
    command_buffer.pipelineBarrier2(vk::DependencyInfo{
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &image_memory_barrier,
    });
}

void transitImageLayout(const vk::CommandBuffer command_buffer, const vk::Image image,
                        const ImageLayoutTransition layout_transition,
                        const ImagePipelineStageTransition stage_transition,
                        const ImageAccessFlagsTransition access_flags_transition,
                        const vk::ImageAspectFlags aspect_flags, const uint32_t mip_levels) {
    const auto [src_access_flag, dst_access_flag] = access_flags_transition;
    const auto [old_layout, new_layout] = layout_transition;
    const auto [src_stages, dst_stages] = stage_transition;
    const auto barrier = vk::ImageMemoryBarrier{ .srcAccessMask = src_access_flag,
                                                 .dstAccessMask = dst_access_flag,
                                                 .oldLayout = old_layout,
                                                 .newLayout = new_layout,
                                                 .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                                                 .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                                                 .image = image,
                                                 .subresourceRange = vk::ImageSubresourceRange{
                                                         .aspectMask = aspect_flags,
                                                         .baseMipLevel = 0u,
                                                         .levelCount = mip_levels,
                                                         .baseArrayLayer = 0u,
                                                         .layerCount = 1u,
                                                 } };
    command_buffer.pipelineBarrier(src_stages, dst_stages, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);
}

void transitImageLayout(const vk::CommandBuffer command_buffer, const vk::Image image,
                        const ImageLayoutTransition layout_transition, const uint32_t mip_levels,
                        const vk::ImageAspectFlags aspect_flags) {
    const auto [old_layout, new_layout] = layout_transition;
    /*if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal) {
        transitImageLayout(command_buffer,
                           image,
                           layout_transition,
                           ImagePipelineStageTransition{ .oldStage = vk::PipelineStageFlagBits::eTopOfPipe,
                                                         .newStage = vk::PipelineStageFlagBits::eTransfer },
                           ImageAccessFlagsTransition{ .oldAccess = vk::AccessFlags(),
                                                       .newAccess = vk::AccessFlagBits::eTransferWrite },
                           aspect_flags,
                           mip_levels);
        return;
    }
    if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        transitImageLayout(command_buffer,
                           image,
                           layout_transition,
                           ImagePipelineStageTransition{ .oldStage = vk::PipelineStageFlagBits::eTransfer,
                                                         .newStage = vk::PipelineStageFlagBits::eFragmentShader },
                           ImageAccessFlagsTransition{ .oldAccess = vk::AccessFlagBits::eTransferWrite,
                                                       .newAccess = vk::AccessFlagBits::eShaderRead },
                           aspect_flags,
                           mip_levels);
        return;
    }
    if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eColorAttachmentOptimal) {
        transitImageLayout(
                command_buffer,
                image,
                layout_transition,
                ImagePipelineStageTransition{ .oldStage = vk::PipelineStageFlagBits::eColorAttachmentOutput
                                                          | vk::PipelineStageFlagBits::eNone,
                                              .newStage = vk::PipelineStageFlagBits::eColorAttachmentOutput },
                ImageAccessFlagsTransition{ .oldAccess = vk::AccessFlagBits::eNone,
                                            .newAccess = vk::AccessFlagBits::eColorAttachmentWrite },
                aspect_flags,
                mip_levels);
        return;
    }
    if (old_layout == vk::ImageLayout::eColorAttachmentOptimal && new_layout == vk::ImageLayout::ePresentSrcKHR) {
        transitImageLayout(command_buffer,
                           image,
                           layout_transition,
                           ImagePipelineStageTransition{ .oldStage = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                                         .newStage = vk::PipelineStageFlagBits::eBottomOfPipe },
                           ImageAccessFlagsTransition{ .oldAccess = vk::AccessFlagBits::eColorAttachmentWrite,
                                                       .newAccess = vk::AccessFlagBits() },
                           aspect_flags,
                           mip_levels);
        return;
    }*/
    transitImageLayout(command_buffer,
                       image,
                       layout_transition,
                       ImagePipelineStageTransition{ .oldStage = vk::PipelineStageFlagBits::eAllCommands,
                                                     .newStage = vk::PipelineStageFlagBits::eAllCommands },
                       ImageAccessFlagsTransition{ .oldAccess = vk::AccessFlagBits::eMemoryWrite,
                                                   .newAccess = vk::AccessFlagBits::eMemoryWrite
                                                                | vk::AccessFlagBits::eMemoryRead },
                       aspect_flags,
                       mip_levels);
}

auto findSupportedImageFormat(const vk::PhysicalDevice device, const std::span<const vk::Format> formats,
                              const vk::ImageTiling image_tiling, const vk::FormatFeatureFlags features) -> vk::Format {
    for (const auto format : formats) {
        const auto properties = device.getFormatProperties(format);
        if (image_tiling == vk::ImageTiling::eLinear && (properties.linearTilingFeatures & features) == features) {
            return format;
        }
        if (image_tiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find image format");
}

void setCommandBufferFrameSize(const vk::CommandBuffer command_buffer, const vk::Extent2D frame_size) {
    command_buffer.setViewport(0,
                               { vk::Viewport(0.0f,
                                              0.0f,
                                              static_cast<float>(frame_size.width),
                                              static_cast<float>(frame_size.height),
                                              0.0f,
                                              1.0f) });
    command_buffer.setScissor(0, { vk::Rect2D(vk::Offset2D(0, 0), frame_size) });
}

}// namespace th
