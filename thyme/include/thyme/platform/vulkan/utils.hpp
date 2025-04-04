#pragma once

#include <numeric>
#include <vector>

#include <fmt/format.h>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.hpp>

#include <thyme/core/common_structs.hpp>
#include <thyme/platform/glfw_window.hpp>
#include <thyme/renderer/structs.hpp>

namespace th::vulkan {

struct UniqueInstanceConfig {
    std::string_view engineName;
    std::string_view appName;
    std::vector<const char*> instanceExtension;
};

class UniqueInstance {
public:
    explicit UniqueInstance(const UniqueInstanceConfig& config);
    static void validateExtensions(const std::vector<const char*>& extensions);
    vk::UniqueInstance instance;

private:
#if !defined(NDEBUG)
    void setupDebugMessenger(const std::vector<const char*>& extensions);
    vk::UniqueDebugUtilsMessengerEXT debugMessenger;
#endif
};

// TODO - the class should support more queue family flags like eSparseBinding
struct QueueFamilyIndices {
    explicit QueueFamilyIndices(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface) noexcept;

    std::optional<uint32_t> graphicFamily;
    std::optional<uint32_t> presentFamily;

    [[nodiscard]] constexpr bool isCompleted() const noexcept {
        return graphicFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSettings {
    vk::SurfaceFormatKHR surfaceFormat;
    vk::PresentModeKHR presetMode;
    uint32_t imageCount;
};

class SwapChainSupportDetails {
public:
    explicit SwapChainSupportDetails(const vk::PhysicalDevice& device, const vk::UniqueSurfaceKHR& surface);

    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;

    [[nodiscard]] inline bool isValid() const noexcept {
        return !formats.empty() && !presentModes.empty();
    }

    [[nodiscard]] inline auto getBestSurfaceFormat() const noexcept -> vk::SurfaceFormatKHR {
        const auto suitableFormat = std::ranges::find_if(formats, [](const vk::SurfaceFormatKHR& format) {
            return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
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

    [[nodiscard]] inline uint32_t getImageCount() const noexcept {
        const auto swapChainImageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && swapChainImageCount > capabilities.maxImageCount) {
            return capabilities.maxImageCount;
        }
        return swapChainImageCount;
    }

    [[nodiscard]] inline auto getSwapExtent(const Resolution& fallbackResolution) const noexcept -> vk::Extent2D {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        const auto [width, height] = fallbackResolution;
        const auto minImageExtent = capabilities.minImageExtent;
        const auto maxImageExtent = capabilities.maxImageExtent;
        return vk::Extent2D{ std::clamp(width, minImageExtent.width, maxImageExtent.width),
                             std::clamp(height, minImageExtent.height, maxImageExtent.height) };
    }

    [[nodiscard]] inline auto getBestSwapChainSettings() const noexcept -> SwapChainSettings {
        return SwapChainSettings{ .surfaceFormat = getBestSurfaceFormat(),
                                  .presetMode = getBestPresetMode(),
                                  .imageCount = getImageCount() };
    }
};

class PhysicalDevice {
public:
    explicit PhysicalDevice(const vk::PhysicalDevice& physicalDevice, const QueueFamilyIndices& queueFamilyIndices,
                            const SwapChainSupportDetails& swapChainSupportDetails,
                            const vk::SampleCountFlagBits maxMsaaSamples) noexcept
        : physicalDevice{ physicalDevice }, queueFamilyIndices{ queueFamilyIndices },
          swapChainSupportDetails{ swapChainSupportDetails }, maxMsaaSamples{ maxMsaaSamples } {}

    vk::PhysicalDevice physicalDevice;
    QueueFamilyIndices queueFamilyIndices;
    SwapChainSupportDetails swapChainSupportDetails;
    vk::SampleCountFlagBits maxMsaaSamples;

    [[nodiscard]] vk::UniqueDevice createLogicalDevice() const;
};

struct Device {
    explicit Device(PhysicalDevice physicalDevice)
        : physicalDevice(physicalDevice.physicalDevice), logicalDevice(physicalDevice.createLogicalDevice()),
          queueFamilyIndices(physicalDevice.queueFamilyIndices),
          swapChainSupportDetails(physicalDevice.swapChainSupportDetails),
          maxMsaaSamples{ physicalDevice.maxMsaaSamples } {}
    vk::PhysicalDevice physicalDevice;
    vk::UniqueDevice logicalDevice;
    QueueFamilyIndices queueFamilyIndices;
    SwapChainSupportDetails swapChainSupportDetails;
    vk::SampleCountFlagBits maxMsaaSamples;

    auto getGraphicQueue() const noexcept -> vk::Queue {
        return logicalDevice->getQueue(queueFamilyIndices.graphicFamily.value(), 0);
    }
};

std::vector<PhysicalDevice> getPhysicalDevices(const vk::UniqueInstance& instance, const vk::UniqueSurfaceKHR& surface);

struct SwapChainFrame {
    vk::Image image;
    vk::UniqueImageView imageView;
    vk::UniqueFramebuffer frameBuffer;
};

class SwapChainData {
public:
    explicit SwapChainData(const Device& device, const SwapChainSettings& swapChainSettings,
                           const vk::Extent2D& swapChainExtent, const vk::UniqueRenderPass& renderPass,
                           const vk::UniqueSurfaceKHR& surface, const vk::UniqueImageView& colorImageView,
                           const vk::UniqueImageView& depthImageView, const vk::SwapchainKHR& oldSwapChain = {});

    vk::UniqueSwapchainKHR swapChain;
    std::vector<SwapChainFrame> swapChainFrame;
};

struct FrameData {
    vk::UniqueCommandBuffer commandBuffer;
    vk::UniqueSemaphore imageAvailableSemaphore;
    vk::UniqueSemaphore renderFinishedSemaphore;
    vk::UniqueFence fence;
    uint32_t currentFrame;
};

[[nodiscard]] inline auto createFrameDataList(const vk::UniqueDevice& logicalDevice,
                                              const vk::UniqueCommandPool& commandPool,
                                              const uint32_t maxFrames) noexcept -> std::vector<FrameData> {
    std::vector<FrameData> frameDataList;
    frameDataList.reserve(maxFrames);
    for (uint32_t i = 0; i < maxFrames; i++) {
        frameDataList.emplace_back(FrameData{
                .commandBuffer = std::move(logicalDevice
                                                   ->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
                                                           *commandPool, vk::CommandBufferLevel::ePrimary, 1))
                                                   .front()),
                .imageAvailableSemaphore = logicalDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo()),
                .renderFinishedSemaphore = logicalDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo()),
                .fence = logicalDevice->createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)),
                .currentFrame = i,
        });
    }
    return frameDataList;
}

class FrameDataList {
public:
    explicit FrameDataList(const vk::UniqueDevice& logicalDevice, const vk::UniqueCommandPool& commandPool,
                           const uint32_t maxFrames) noexcept {
        m_frameDataList = createFrameDataList(logicalDevice, commandPool, maxFrames);
    }

    [[nodiscard]] auto getNext() noexcept -> const FrameData& {
        const auto index = getNextFrameIndex();
        return m_frameDataList[index];
    };

private:
    std::vector<FrameData> m_frameDataList;

    uint32_t frameIndex{ 0 };

    [[nodiscard]] uint32_t getNextFrameIndex() noexcept {
        const auto currentFrameIndex = frameIndex;
        frameIndex = (frameIndex + 1) % m_frameDataList.size();
        return currentFrameIndex;
    };
};

[[nodiscard]] auto createRenderPass(const vk::UniqueDevice& logicalDevice, const vk::Format colorFormat,
                                    const vk::Format depthFormat, const vk::SampleCountFlagBits samples)
        -> vk::UniqueRenderPass;

struct GraphicPipelineCreateInfo {
    const vk::UniqueDevice& logicalDevice;
    const vk::UniqueRenderPass& renderPass;
    const vk::UniquePipelineLayout& pipelineLayout;
    vk::SampleCountFlagBits samples;
    const std::vector<vk::PipelineShaderStageCreateInfo>& shaderStages;
};

[[nodiscard]] auto createGraphicsPipeline(const GraphicPipelineCreateInfo& graphicPipelineCreateInfo)
        -> vk::UniquePipeline;

[[nodiscard]] inline auto createDescriptorPool(const vk::UniqueDevice& device,
                                               const std::vector<vk::DescriptorPoolSize>& descriptorSizes)
        -> vk::UniqueDescriptorPool {
    const auto maxSet = std::accumulate(std::begin(descriptorSizes),
                                        std::end(descriptorSizes),
                                        uint32_t{ 0 },
                                        [](const uint32_t sum, const vk::DescriptorPoolSize& descriptorPoolSize) {
                                            return sum + descriptorPoolSize.descriptorCount;
                                        });

    return device->createDescriptorPoolUnique(
            vk::DescriptorPoolCreateInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                                         maxSet,
                                         static_cast<uint32_t>(descriptorSizes.size()),
                                         descriptorSizes.data()));
}
// TODO - can it be done much better??
template <typename F, typename... Args>
    requires(std::invocable<F, const vk::UniqueCommandBuffer&, Args...>)
void singleTimeCommand(const vk::UniqueCommandBuffer& commandBuffer, const vk::Queue& graphicQueue, F fun,
                       Args... args) {
    commandBuffer->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    fun(commandBuffer, args...);
    commandBuffer->end();
    graphicQueue.submit(vk::SubmitInfo({}, {}, { commandBuffer.get() }), nullptr);
    graphicQueue.waitIdle();
}

template <typename F, typename... Args>
    requires(std::invocable<F, const vk::UniqueCommandBuffer&, Args...>)
void singleTimeCommand(const vk::UniqueDevice& device, const vk::UniqueCommandPool& commandPool,
                       const vk::Queue& graphicQueue, F fun, Args... args) {
    auto commandBuffer = std::move(
            device->allocateCommandBuffersUnique(
                          vk::CommandBufferAllocateInfo(commandPool.get(), vk::CommandBufferLevel::ePrimary, 1))
                    .front());
    singleTimeCommand(commandBuffer, graphicQueue, fun, args...);
}

template <typename F, typename... Args>
    requires(std::invocable<F, const vk::UniqueCommandBuffer&, Args...>)
void singleTimeCommand(const Device& device, const vk::UniqueCommandPool& commandPool, F fun, Args... args) {

    auto commandBuffer = std::move(device.logicalDevice
                                           ->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
                                                   commandPool.get(), vk::CommandBufferLevel::ePrimary, 1))
                                           .front());
    const auto graphicQueue = device.getGraphicQueue();
    singleTimeCommand(commandBuffer, graphicQueue, fun, args...);
}

static constexpr auto getBindingDescription() -> vk::VertexInputBindingDescription {
    return vk::VertexInputBindingDescription(0, sizeof(th::renderer::Vertex), vk::VertexInputRate::eVertex);
}

static constexpr auto getAttributeDescriptions() -> std::array<vk::VertexInputAttributeDescription, 3> {
    using namespace th::renderer;
    return std::array{
        vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
        vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
        vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)),
    };
}


[[nodiscard]] inline uint32_t findMemoryType(const vk::PhysicalDevice& device, const uint32_t typeFilter,
                                             const vk::MemoryPropertyFlags properties) {
    const auto& memProperties = device.getMemoryProperties();
    for (uint32_t i{ 0 }; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

inline void copyBuffer(const vk::UniqueDevice& device, const vk::UniqueCommandPool& commandPool,
                       const vk::Queue& graphicQueue, const vk::UniqueBuffer& srcBuffer,
                       const vk::UniqueBuffer& dstBuffer, const size_t size) {
    singleTimeCommand(device, commandPool, graphicQueue, [&](const vk::UniqueCommandBuffer& commandBuffer) {
        commandBuffer->copyBuffer(*srcBuffer, *dstBuffer, { vk::BufferCopy(0, 0, size) });
    });
}


class BufferMemory {
public:
    BufferMemory(const Device& device, const size_t size, const vk::BufferUsageFlags usage,
                 const vk::MemoryPropertyFlags properties);

    template <typename Vec>
    BufferMemory(const Device& device, const vk::UniqueCommandPool& commandPool, const Vec& data,
                 const vk::BufferUsageFlags usage)
        : BufferMemory(device, data.size() * sizeof(data[0]), vk::BufferUsageFlagBits::eTransferDst | usage,
                       vk::MemoryPropertyFlagBits::eDeviceLocal) {
        const auto size = data.size() * sizeof(data[0]);

        const auto stagingMemoryBuffer =
                BufferMemory(device,
                             size,
                             vk::BufferUsageFlagBits::eTransferSrc,
                             vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        void* mappedMemory = nullptr;
        [[maybe_unused]] const auto result = device.logicalDevice->mapMemory(
                *stagingMemoryBuffer.getMemory(), 0, size, vk::MemoryMapFlags(), &mappedMemory);
        memcpy(mappedMemory, data.data(), size);
        device.logicalDevice->unmapMemory(stagingMemoryBuffer.getMemory().get());
        const auto& graphicQueue = device.logicalDevice->getQueue(device.queueFamilyIndices.graphicFamily.value(), 0);
        copyBuffer(device.logicalDevice, commandPool, graphicQueue, stagingMemoryBuffer.getBuffer(), m_buffer, size);
    }
    [[nodiscard]] auto getBuffer() const noexcept -> const vk::UniqueBuffer& {
        return m_buffer;
    }
    [[nodiscard]] auto getMemory() const noexcept -> const vk::UniqueDeviceMemory& {
        return m_memory;
    }

private:
    vk::UniqueBuffer m_buffer;
    vk::UniqueDeviceMemory m_memory;
};

inline void transitImageLayout(const Device& device, const vk::UniqueCommandPool& commandPool,
                               const vk::UniqueImage& image, const vk::ImageLayout oldLayout,
                               const vk::ImageLayout newLayout, const uint32_t mipLevels);

inline void copyBufferToImage(const Device& device, const vk::UniqueCommandPool& commandPool,
                              const vk::UniqueBuffer& buffer, const vk::UniqueImage& image,
                              const Resolution& resolution) {
    singleTimeCommand(device, commandPool, [&](const vk::UniqueCommandBuffer& commandBuffer) {
        const auto region = vk::BufferImageCopy(0,
                                                0,
                                                0,
                                                vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                                                vk::Offset3D(0, 0, 0),
                                                vk::Extent3D(resolution.width, resolution.height, 1));
        commandBuffer->copyBufferToImage(*buffer, *image, vk::ImageLayout::eTransferDstOptimal, region);
    });
}

class ImageMemory {
public:
    ImageMemory(const Device& device, const Resolution& resolution, const vk::Format format,
                const vk::ImageUsageFlags imageUsageFlags, const vk::MemoryPropertyFlags memoryPropertyFlags,
                const vk::ImageAspectFlags aspectFlags, const vk::SampleCountFlagBits msaa, const uint32_t mipLevels);

    ImageMemory(const Device& device, const vk::UniqueCommandPool& commandPool, const std::span<const uint8_t> data,
                const Resolution& resolution, const vk::SampleCountFlagBits msaa, const uint32_t mipLevels = 1);

    [[nodiscard]] auto getImage() const noexcept -> const vk::UniqueImage& {
        return m_image;
    }

    [[nodiscard]] auto getMemory() const noexcept -> const vk::UniqueDeviceMemory& {
        return m_memory;
    }

    [[nodiscard]] auto getImageView() const noexcept -> const vk::UniqueImageView& {
        return m_imageView;
    }

private:
    vk::UniqueImage m_image;
    vk::UniqueDeviceMemory m_memory;
    vk::UniqueImageView m_imageView;
};

void generateMipmaps(const Device& device, const vk::UniqueCommandPool& commandPool, const vk::UniqueImage& image,
                     const vk::Format format, const Resolution& resolution, const uint32_t mipLevels);

[[nodiscard]] inline auto createImageSampler(const Device& device, const uint32_t mipLevels) noexcept
        -> vk::UniqueSampler {
    return device.logicalDevice->createSamplerUnique(
            vk::SamplerCreateInfo(vk::SamplerCreateFlags(),
                                  vk::Filter::eLinear,
                                  vk::Filter::eLinear,
                                  vk::SamplerMipmapMode::eLinear,
                                  vk::SamplerAddressMode::eRepeat,
                                  vk::SamplerAddressMode::eRepeat,
                                  vk::SamplerAddressMode::eRepeat,
                                  0.0f,
                                  vk::True,
                                  device.physicalDevice.getProperties().limits.maxSamplerAnisotropy,
                                  vk::False,
                                  vk::CompareOp::eAlways,
                                  0.0f,
                                  static_cast<float>(mipLevels),
                                  vk::BorderColor::eIntOpaqueBlack,
                                  vk::False));
}

[[nodiscard]] auto findSupportedImageFormat(const vk::PhysicalDevice& device,
                                            const std::span<const vk::Format> formats,
                                            const vk::ImageTiling imageTiling,
                                            const vk::FormatFeatureFlags features) -> vk::Format;

[[nodiscard]] inline auto findDepthFormat(const vk::PhysicalDevice& device) -> vk::Format {
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

}// namespace th::vulkan
