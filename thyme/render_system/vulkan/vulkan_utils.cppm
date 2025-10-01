module;

#include <glm/glm.hpp>

export module th.render_system.vulkan:utils;

import std;

import vulkan_hpp;

import th.scene.model;

export namespace th {

struct QueueFamilyIndices {
    explicit QueueFamilyIndices(vk::PhysicalDevice device, std::optional<vk::SurfaceKHR> surface);

    std::optional<std::uint32_t> graphicFamily;
    std::optional<std::uint32_t> presentFamily;

    [[nodiscard]] constexpr auto isCompleted() const noexcept -> bool {
        return graphicFamily.has_value() && (m_requested_surface_support ? presentFamily.has_value() : true);
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

    [[nodiscard]] bool isValid() const noexcept {
        return !formats.empty() && !presentModes.empty();
    }

    [[nodiscard]] auto getBestSurfaceFormat() const noexcept -> vk::SurfaceFormatKHR {
        const auto suitableFormat = std::ranges::find_if(formats, [](const vk::SurfaceFormatKHR& format) {
            return format.format == vk::Format::eB8G8R8A8Unorm
                   && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
        if (suitableFormat != formats.end()) {
            return *suitableFormat;
        }
        return formats[0];
    }

    [[nodiscard]] inline auto getBestPresetMode() const noexcept -> vk::PresentModeKHR {
        const auto suitablePreset = std::ranges::find_if(presentModes, [](const vk::PresentModeKHR presentMode) {
            return presentMode == vk::PresentModeKHR::eMailbox;
        });

        if (suitablePreset != presentModes.end()) {
            return *suitablePreset;
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
                                               const std::vector<vk::DescriptorPoolSize>& descriptorSizes)
        -> vk::UniqueDescriptorPool {
    const auto maxSet = std::accumulate(std::begin(descriptorSizes),
                                        std::end(descriptorSizes),
                                        uint32_t{ 0 },
                                        [](const uint32_t sum, const vk::DescriptorPoolSize& descriptorPoolSize) {
                                            return sum + descriptorPoolSize.descriptorCount;
                                        });


    return device.createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = maxSet,
            .poolSizeCount = static_cast<uint32_t>(descriptorSizes.size()),
            .pPoolSizes = descriptorSizes.data(),
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
        vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)),
    };
}


[[nodiscard]] inline auto findMemoryType(const vk::PhysicalDevice device, const uint32_t typeFilter,
                                         const vk::MemoryPropertyFlags properties) -> uint32_t {
    const auto& memProperties = device.getMemoryProperties();
    for (uint32_t i{ 0 }; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

inline void copyBuffer(const vk::Device device, const vk::CommandPool commandPool, const vk::Queue graphicQueue,
                       const vk::Buffer srcBuffer, const vk::Buffer dstBuffer, const size_t size) {
    singleTimeCommand(device, commandPool, graphicQueue, [&](const vk::CommandBuffer& commandBuffer) {
        commandBuffer.copyBuffer(srcBuffer, dstBuffer, { vk::BufferCopy(0, 0, size) });
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

void transitImageLayout(vk::CommandBuffer commandBuffer, vk::Image image, ImageLayoutTransition layoutTransition,
                        uint32_t mipLevels);

void transitImageLayout(vk::CommandBuffer commandBuffer, vk::Image image, ImageLayoutTransition layoutTransition,
                        ImagePipelineStageTransition stageTransition, ImageAccessFlagsTransition accessFlagsTransition,
                        vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);

inline void copyBufferToImage(vk::Device device, const vk::CommandPool commandPool, const vk::Queue graphicQueue,
                              const vk::Buffer buffer, const vk::Image image, const glm::uvec2& resolution) {
    singleTimeCommand(device, commandPool, graphicQueue, [&](const vk::CommandBuffer commandBuffer) {
        const auto region = vk::BufferImageCopy(0,
                                                0,
                                                0,
                                                vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                                                vk::Offset3D(0, 0, 0),
                                                vk::Extent3D(resolution.x, resolution.y, 1));
        commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
    });
}

[[nodiscard]] auto findSupportedImageFormat(vk::PhysicalDevice device,
                                            std::span<const vk::Format> formats,
                                            vk::ImageTiling imageTiling,
                                            vk::FormatFeatureFlags features) -> vk::Format;

[[nodiscard]] inline auto findDepthFormat(const vk::PhysicalDevice device) -> vk::Format {
    constexpr auto formats =
            std::array{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint };
    return findSupportedImageFormat(device,
                                    std::span(formats.data(), formats.size()),
                                    vk::ImageTiling::eOptimal,
                                    vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

[[nodiscard]] inline bool hasStencilFormat(const vk::Format format) noexcept {
    return format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD32SfloatS8Uint;
}

void setCommandBufferFrameSize(vk::CommandBuffer commandBuffer, vk::Extent2D frameSize);

QueueFamilyIndices::QueueFamilyIndices(const vk::PhysicalDevice device, const std::optional<vk::SurfaceKHR> surface)
    : m_requested_surface_support{ surface.has_value() } {
    const auto& queueFamilies = device.getQueueFamilyProperties2();
    for (uint32_t i{ 0 }; i < queueFamilies.size(); i++) {
        const auto& queueFamily = queueFamilies[i];
        const auto& queueFamilyProperties = queueFamily.queueFamilyProperties;
        if (queueFamilyProperties.queueCount <= 0) {
            continue;
        }
        if (queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicFamily = i;
        }
        if (queueFamilyProperties.queueFlags & vk::QueueFlagBits::eCompute) {}
        if (queueFamilyProperties.queueFlags & vk::QueueFlagBits::eTransfer) {}
        if (surface.has_value() ? device.getSurfaceSupportKHR(i, surface.value()) : true) {
            presentFamily = i;
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

void transitImageLayout(const vk::CommandBuffer commandBuffer, const vk::Image image,
                        const ImageLayoutTransition layoutTransition,
                        const ImagePipelineStageTransition stageTransition,
                        const ImageAccessFlagsTransition accessFlagsTransition, const vk::ImageAspectFlags aspectFlags,
                        const uint32_t mipLevels) {
    const auto [srcAccessFlag, dstAccessFlag] = accessFlagsTransition;
    const auto [oldLayout, newLayout] = layoutTransition;
    const auto [srcStages, dstStages] = stageTransition;
    const auto barrier = vk::ImageMemoryBarrier{ .srcAccessMask = srcAccessFlag,
                                                 .dstAccessMask = dstAccessFlag,
                                                 .oldLayout = oldLayout,
                                                 .newLayout = newLayout,
                                                 .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                                                 .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                                                 .image = image,
                                                 .subresourceRange = vk::ImageSubresourceRange{
                                                         .aspectMask = aspectFlags,
                                                         .baseMipLevel = 0u,
                                                         .levelCount = mipLevels,
                                                         .baseArrayLayer = 0u,
                                                         .layerCount = 1u,
                                                 } };
    commandBuffer.pipelineBarrier(srcStages, dstStages, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);
}

void transitImageLayout(const vk::CommandBuffer commandBuffer, const vk::Image image,
                        const ImageLayoutTransition layoutTransition, const uint32_t mipLevels) {
    const auto [oldLayout, newLayout] = layoutTransition;
    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        transitImageLayout(commandBuffer,
                           image,
                           layoutTransition,
                           ImagePipelineStageTransition{ .oldStage = vk::PipelineStageFlagBits::eTopOfPipe,
                                                         .newStage = vk::PipelineStageFlagBits::eTransfer },
                           ImageAccessFlagsTransition{ .oldAccess = vk::AccessFlags(),
                                                       .newAccess = vk::AccessFlagBits::eTransferWrite },
                           vk::ImageAspectFlagBits::eColor,
                           mipLevels);
        return;
    }
    if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        transitImageLayout(commandBuffer,
                           image,
                           layoutTransition,
                           ImagePipelineStageTransition{ .oldStage = vk::PipelineStageFlagBits::eTransfer,
                                                         .newStage = vk::PipelineStageFlagBits::eFragmentShader },
                           ImageAccessFlagsTransition{ .oldAccess = vk::AccessFlagBits::eTransferWrite,
                                                       .newAccess = vk::AccessFlagBits::eShaderRead },
                           vk::ImageAspectFlagBits::eColor,
                           mipLevels);
        return;
    }
    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        transitImageLayout(
                commandBuffer,
                image,
                layoutTransition,
                ImagePipelineStageTransition{ .oldStage = vk::PipelineStageFlagBits::eColorAttachmentOutput
                                                          | vk::PipelineStageFlagBits::eNone,
                                              .newStage = vk::PipelineStageFlagBits::eColorAttachmentOutput },
                ImageAccessFlagsTransition{ .oldAccess = vk::AccessFlagBits::eNone,
                                            .newAccess = vk::AccessFlagBits::eColorAttachmentWrite },
                vk::ImageAspectFlagBits::eColor,
                mipLevels);
        return;
    }
    if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal && newLayout == vk::ImageLayout::ePresentSrcKHR) {
        transitImageLayout(commandBuffer,
                           image,
                           layoutTransition,
                           ImagePipelineStageTransition{ .oldStage = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                                         .newStage = vk::PipelineStageFlagBits::eBottomOfPipe },
                           ImageAccessFlagsTransition{ .oldAccess = vk::AccessFlagBits::eColorAttachmentWrite,
                                                       .newAccess = vk::AccessFlagBits() },
                           vk::ImageAspectFlagBits::eColor,
                           mipLevels);
        return;
    }
    transitImageLayout(commandBuffer,
                       image,
                       layoutTransition,
                       ImagePipelineStageTransition{ .oldStage = vk::PipelineStageFlagBits::eAllCommands,
                                                     .newStage = vk::PipelineStageFlagBits::eAllCommands },
                       ImageAccessFlagsTransition{ .oldAccess = vk::AccessFlagBits::eMemoryWrite,
                                                   .newAccess = vk::AccessFlagBits::eMemoryWrite
                                                                | vk::AccessFlagBits::eMemoryRead },
                       vk::ImageAspectFlagBits::eColor,
                       mipLevels);
}

auto findSupportedImageFormat(const vk::PhysicalDevice device, const std::span<const vk::Format> formats,
                              const vk::ImageTiling imageTiling, const vk::FormatFeatureFlags features) -> vk::Format {
    for (const auto format : formats) {
        const auto properties = device.getFormatProperties(format);
        if (imageTiling == vk::ImageTiling::eLinear && (properties.linearTilingFeatures & features) == features) {
            return format;
        }
        if (imageTiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find image format");
}

void setCommandBufferFrameSize(const vk::CommandBuffer commandBuffer, const vk::Extent2D frameSize) {
    commandBuffer.setViewport(0,
                              { vk::Viewport(0.0f,
                                             0.0f,
                                             static_cast<float>(frameSize.width),
                                             static_cast<float>(frameSize.height),
                                             0.0f,
                                             1.0f) });
    commandBuffer.setScissor(0, { vk::Rect2D(vk::Offset2D(0, 0), frameSize) });
}

}// namespace th::render_system::vulkan
