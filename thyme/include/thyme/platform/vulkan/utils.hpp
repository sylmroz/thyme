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

    [[nodiscard]] auto getInstance() const noexcept -> vk::Instance {
        return m_instance.get();
    }

private:
    vk::UniqueInstance m_instance;

private:
#if !defined(NDEBUG)
    void setupDebugMessenger(const std::vector<const char*>& extensions);
    vk::UniqueDebugUtilsMessengerEXT debugMessenger;
#endif
};

// TODO - the class should support more queue family flags like eSparseBinding
struct QueueFamilyIndices {
    explicit QueueFamilyIndices(const vk::PhysicalDevice& device, const vk::SurfaceKHR surface) noexcept;

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
    explicit SwapChainSupportDetails(const vk::PhysicalDevice& device, const vk::SurfaceKHR surface);

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
          maxMsaaSamples{ physicalDevice.maxMsaaSamples },
          commandPool{ logicalDevice->createCommandPoolUnique(
                  vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                            queueFamilyIndices.graphicFamily.value())) } {}
    vk::PhysicalDevice physicalDevice;
    vk::UniqueDevice logicalDevice;
    QueueFamilyIndices queueFamilyIndices;
    SwapChainSupportDetails swapChainSupportDetails;
    vk::SampleCountFlagBits maxMsaaSamples;
    vk::UniqueCommandPool commandPool;

    auto getGraphicQueue() const noexcept -> vk::Queue {
        return logicalDevice->getQueue(queueFamilyIndices.graphicFamily.value(), 0);
    }
};

std::vector<PhysicalDevice> getPhysicalDevices(const vk::Instance instance, const vk::SurfaceKHR surface);

struct FrameData {
    vk::UniqueSemaphore imageAvailableSemaphore;
    vk::UniqueSemaphore renderFinishedSemaphore;
    vk::UniqueFence fence;
};

struct FrameDataNoUnique {
    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderFinishedSemaphore;
    vk::Fence fence;
};

class FrameDataList {
public:
    explicit FrameDataList(const vk::Device logicalDevice, const vk::CommandPool commandPool,
                           const uint32_t maxFrames) noexcept;

    [[nodiscard]] auto getNext() noexcept -> FrameDataNoUnique {
        const auto index = getNextFrameIndex();
        return FrameDataNoUnique{
            .imageAvailableSemaphore = m_frameDataList[index].imageAvailableSemaphore.get(),
            .renderFinishedSemaphore = m_frameDataList[index].renderFinishedSemaphore.get(),
            .fence = m_frameDataList[index].fence.get(),
        };
    };

private:
    std::vector<FrameData> m_frameDataList;
    uint32_t m_frameIndex{ 0 };

    [[nodiscard]] uint32_t getNextFrameIndex() noexcept {
        const auto currentFrameIndex = m_frameIndex;
        m_frameIndex = (m_frameIndex + 1) % m_frameDataList.size();
        return currentFrameIndex;
    };
};

[[nodiscard]] auto createRenderPass(const vk::Device logicalDevice, const vk::Format colorFormat,
                                    const vk::Format depthFormat, const vk::SampleCountFlagBits samples)
        -> vk::UniqueRenderPass;

struct GraphicPipelineCreateInfo {
    vk::Device logicalDevice;
    vk::RenderPass renderPass;
    vk::PipelineLayout pipelineLayout;
    vk::SampleCountFlagBits samples;
    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo;
    const std::vector<vk::PipelineShaderStageCreateInfo>& shaderStages;
};

[[nodiscard]] auto createGraphicsPipeline(const GraphicPipelineCreateInfo& graphicPipelineCreateInfo)
        -> vk::UniquePipeline;

[[nodiscard]] inline auto createDescriptorPool(const vk::Device device,
                                               const std::vector<vk::DescriptorPoolSize>& descriptorSizes)
        -> vk::UniqueDescriptorPool {
    const auto maxSet = std::accumulate(std::begin(descriptorSizes),
                                        std::end(descriptorSizes),
                                        uint32_t{ 0 },
                                        [](const uint32_t sum, const vk::DescriptorPoolSize& descriptorPoolSize) {
                                            return sum + descriptorPoolSize.descriptorCount;
                                        });

    return device.createDescriptorPoolUnique(
            vk::DescriptorPoolCreateInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                                         maxSet,
                                         static_cast<uint32_t>(descriptorSizes.size()),
                                         descriptorSizes.data()));
}

template <typename F, typename... Args>
    requires(std::invocable<F, const vk::CommandBuffer, Args...>)
void singleTimeCommand(const vk::CommandBuffer commandBuffer, const vk::Queue graphicQueue, F fun, Args... args) {
    commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    fun(commandBuffer, args...);
    commandBuffer.end();
    graphicQueue.submit(vk::SubmitInfo({}, {}, { commandBuffer }), nullptr);
    graphicQueue.waitIdle();
}

template <typename F, typename... Args>
    requires(std::invocable<F, const vk::CommandBuffer, Args...>)
void singleTimeCommand(const vk::Device device, const vk::CommandPool commandPool, const vk::Queue graphicQueue, F fun,
                       Args... args) {
    const auto commandBuffer =
            std::move(device.allocateCommandBuffersUnique(
                                    vk::CommandBufferAllocateInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1))
                              .front());
    singleTimeCommand(commandBuffer.get(), graphicQueue, fun, args...);
}

template <typename F, typename... Args>
    requires(std::invocable<F, const vk::CommandBuffer, Args...>)
void singleTimeCommand(const Device& device, const vk::CommandPool commandPool, F fun, Args... args) {

    const auto commandBuffer = std::move(device.logicalDevice
                                                 ->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
                                                         commandPool, vk::CommandBufferLevel::ePrimary, 1))
                                                 .front());
    const auto graphicQueue = device.getGraphicQueue();
    singleTimeCommand(commandBuffer.get(), graphicQueue, fun, args...);
}

template <typename F, typename... Args>
    requires(std::invocable<F, const vk::CommandBuffer, Args...>)
void singleTimeCommand(const Device& device, F fun, Args... args) {

    const auto commandBuffer = std::move(device.logicalDevice
                                                 ->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
                                                         device.commandPool.get(), vk::CommandBufferLevel::ePrimary, 1))
                                                 .front());
    const auto graphicQueue = device.getGraphicQueue();
    singleTimeCommand(commandBuffer.get(), graphicQueue, fun, args...);
}

static constexpr auto getBindingDescription() -> vk::VertexInputBindingDescription {
    return { 0, sizeof(th::renderer::Vertex), vk::VertexInputRate::eVertex };
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

inline void copyBuffer(const vk::Device device, const vk::CommandPool commandPool, const vk::Queue graphicQueue,
                       const vk::Buffer srcBuffer, const vk::Buffer dstBuffer, const size_t size) {
    singleTimeCommand(device, commandPool, graphicQueue, [&](const vk::CommandBuffer& commandBuffer) {
        commandBuffer.copyBuffer(srcBuffer, dstBuffer, { vk::BufferCopy(0, 0, size) });
    });
}


class BufferMemory {
public:
    BufferMemory(const Device& device, size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    BufferMemory(vk::Device device, vk::PhysicalDevice physicalDevice, size_t size, vk::BufferUsageFlags usage,
                 vk::MemoryPropertyFlags properties);

    template <typename Vec>
    BufferMemory(const vk::Device device, vk::PhysicalDevice physicalDevice, const vk::Queue graphicQueue,
                 const vk::CommandPool commandPool, const Vec& data, const vk::BufferUsageFlags usage)
        : BufferMemory(device, physicalDevice, data.size() * sizeof(data[0]),
                       vk::BufferUsageFlagBits::eTransferDst | usage, vk::MemoryPropertyFlagBits::eDeviceLocal) {
        const auto size = data.size() * sizeof(data[0]);

        const auto stagingMemoryBuffer =
                BufferMemory(device,
                             physicalDevice,
                             size,
                             vk::BufferUsageFlagBits::eTransferSrc,
                             vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        void* mappedMemory = nullptr;
        [[maybe_unused]] const auto result =
                device.mapMemory(stagingMemoryBuffer.getMemory().get(), 0, size, vk::MemoryMapFlags(), &mappedMemory);
        memcpy(mappedMemory, data.data(), size);
        device.unmapMemory(stagingMemoryBuffer.getMemory().get());
        copyBuffer(device, commandPool, graphicQueue, stagingMemoryBuffer.getBuffer().get(), m_buffer.get(), size);
    }

    template <typename Vec>
    BufferMemory(const Device& device, const Vec& data, const vk::BufferUsageFlags usage)
        : BufferMemory(device.logicalDevice.get(), device.physicalDevice, device.getGraphicQueue(),
                       device.commandPool.get(), data, usage) {}

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

void transitImageLayout(const Device& device, const vk::CommandPool commandPool, const vk::Image image,
                        const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout, const uint32_t mipLevels);

void transitImageLayout(const vk::Device device, const vk::CommandPool commandPool, const vk::Queue graphicQueue,
                        const vk::Image image, const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout,
                        const uint32_t mipLevels);

void transitImageLayout(const Device& device, const vk::Image image, const vk::ImageLayout oldLayout,
                        const vk::ImageLayout newLayout, const uint32_t mipLevels);

void transitImageLayout(const vk::CommandBuffer commandBuffer, const vk::Image image, const vk::ImageLayout oldLayout,
                        const vk::ImageLayout newLayout, const uint32_t mipLevels);

void transitImageLayout(const vk::CommandBuffer commandBuffer, const vk::Image image,
                        const ImageLayoutTransition layoutTransition,
                        const ImagePipelineStageTransition stageTransition,
                        const ImageAccessFlagsTransition accessFlagsTransition, const vk::ImageAspectFlags aspectFlags,
                        const uint32_t mipLevels);

inline void copyBufferToImage(const vk::Device device, const vk::CommandPool commandPool, vk::Queue graphicQueue,
                              const vk::Buffer buffer, const vk::Image image, const Resolution resolution) {
    singleTimeCommand(device, commandPool, graphicQueue, [&](const vk::CommandBuffer commandBuffer) {
        const auto region = vk::BufferImageCopy(0,
                                                0,
                                                0,
                                                vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                                                vk::Offset3D(0, 0, 0),
                                                vk::Extent3D(resolution.width, resolution.height, 1));
        commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
    });
}

inline void copyBufferToImage(const Device& device, const vk::Buffer buffer, const vk::Image image,
                              const Resolution resolution) {
    copyBufferToImage(
            device.logicalDevice.get(), device.commandPool.get(), device.getGraphicQueue(), buffer, image, resolution);
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

void setCommandBufferFrameSize(const vk::CommandBuffer commandBuffer, const vk::Extent2D frameSize);

}// namespace th::vulkan
