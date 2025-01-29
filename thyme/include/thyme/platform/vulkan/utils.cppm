module;

#include <numeric>
#include <vector>

#include <fmt/format.h>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.hpp>

export module thyme.platform.vulkan:utils;
import thyme.core.common_structs;
import thyme.platform.glfw_window;

export namespace Thyme::Vulkan {

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
                            const SwapChainSupportDetails& swapChainSupportDetails) noexcept
        : physicalDevice{ physicalDevice }, queueFamilyIndices{ queueFamilyIndices },
          swapChainSupportDetails{ swapChainSupportDetails } {}

    vk::PhysicalDevice physicalDevice;
    QueueFamilyIndices queueFamilyIndices;
    SwapChainSupportDetails swapChainSupportDetails;

    [[nodiscard]] vk::UniqueDevice createLogicalDevice() const;
};

struct Device {
    explicit Device(PhysicalDevice physicalDevice)
        : physicalDevice(physicalDevice.physicalDevice), logicalDevice(physicalDevice.createLogicalDevice()),
          queueFamilyIndices(physicalDevice.queueFamilyIndices),
          swapChainSupportDetails(physicalDevice.swapChainSupportDetails) {}
    vk::PhysicalDevice physicalDevice;
    vk::UniqueDevice logicalDevice;
    QueueFamilyIndices queueFamilyIndices;
    SwapChainSupportDetails swapChainSupportDetails;

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
                           const vk::UniqueSurfaceKHR& surface, const vk::UniqueImageView& depthImageView,
                           const vk::SwapchainKHR& oldSwapChain = {});

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
                                    const vk::Format depthFormat) -> vk::UniqueRenderPass;

struct GraphicPipelineCreateInfo {
    const vk::UniqueDevice& logicalDevice;
    const vk::UniqueRenderPass& renderPass;
    const vk::UniquePipelineLayout& pipelineLayout;
    vk::SampleCountFlagBits samples;
    const std::vector<vk::PipelineShaderStageCreateInfo>& shaderStages;
};

[[nodiscard]] auto createGraphicsPipeline(const GraphicPipelineCreateInfo& graphicPipelineCreateInfo)
        -> vk::UniquePipeline;

[[nodiscard]] auto createDescriptorPool(const vk::UniqueDevice& device,
                                        const std::vector<vk::DescriptorPoolSize>& descriptorSizes)
        -> vk::UniqueDescriptorPool {
    const uint32_t maxSet = std::accumulate(std::begin(descriptorSizes),
                                            std::end(descriptorSizes),
                                            0,
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
void singleTimeCommand(const vk::UniqueCommandBuffer& commandBuffer, const vk::UniqueCommandPool& commandPool,
                       const vk::Queue& graphicQueue, F fun, Args... args) {
    commandBuffer->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    fun(commandBuffer, args...);
    commandBuffer->end();
    graphicQueue.submit(vk::SubmitInfo(0, nullptr, nullptr, 1, &(commandBuffer.get())), nullptr);
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
    singleTimeCommand(commandBuffer, commandPool, graphicQueue, fun, args...);
}

template <typename F, typename... Args>
    requires(std::invocable<F, const vk::UniqueCommandBuffer&, Args...>)
void singleTimeCommand(const Device& device, const vk::UniqueCommandPool& commandPool, F fun, Args... args) {

    auto commandBuffer = std::move(device.logicalDevice
                                           ->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
                                                   commandPool.get(), vk::CommandBufferLevel::ePrimary, 1))
                                           .front());
    const auto graphicQueue = device.getGraphicQueue();
    singleTimeCommand(commandBuffer, commandPool, graphicQueue, fun, args...);
}

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static constexpr auto getBindingDescription() -> vk::VertexInputBindingDescription {
        return vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
    }

    static constexpr auto getAttributeDescriptions() -> std::array<vk::VertexInputAttributeDescription, 3> {
        return std::array{
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)),
        };
    }
};

[[nodiscard]] uint32_t findMemoryType(const vk::PhysicalDevice& device, const uint32_t typeFilter,
                                      const vk::MemoryPropertyFlags properties) {
    const auto& memProperties = device.getMemoryProperties();
    for (uint32_t i{ 0 }; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

void copyBuffer(const vk::UniqueDevice& device, const vk::UniqueCommandPool& commandPool, const vk::Queue& graphicQueue,
                const vk::UniqueBuffer& srcBuffer, const vk::UniqueBuffer& dstBuffer, const uint32_t size) {
    singleTimeCommand(device, commandPool, graphicQueue, [&](const vk::UniqueCommandBuffer& commandBuffer) {
        commandBuffer->copyBuffer(*srcBuffer, *dstBuffer, { vk::BufferCopy(0, 0, size) });
    });
}


struct BufferMemory {
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
};

[[nodiscard]] BufferMemory createBufferMemory(const Device& device, const uint32_t size,
                                              const vk::BufferUsageFlags usage,
                                              const vk::MemoryPropertyFlags properties) {
    auto buffer = device.logicalDevice->createBufferUnique(
            vk::BufferCreateInfo(vk::BufferCreateFlagBits(), size, usage, vk::SharingMode::eExclusive));

    vk::MemoryRequirements memoryRequirements;
    device.logicalDevice->getBufferMemoryRequirements(*buffer, &memoryRequirements);

    auto memory = device.logicalDevice->allocateMemoryUnique(vk::MemoryAllocateInfo(
            memoryRequirements.size,
            findMemoryType(device.physicalDevice, memoryRequirements.memoryTypeBits, properties)));

    device.logicalDevice->bindBufferMemory(*buffer, *memory, 0);

    return BufferMemory{ .buffer = std::move(buffer), .memory = std::move(memory) };
}

template <typename Vec>
[[nodiscard]] BufferMemory createBufferMemory(const Device& device, const vk::UniqueCommandPool& commandPool,
                                              const Vec& data, const vk::BufferUsageFlags usage) {
    const auto size = data.size() * sizeof(data[0]);

    const auto stagingMemoryBuffer =
            createBufferMemory(device,
                               size,
                               vk::BufferUsageFlagBits::eTransferSrc,
                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* mappedMemory = nullptr;
    [[maybe_unused]] const auto result =
            device.logicalDevice->mapMemory(*stagingMemoryBuffer.memory, 0, size, vk::MemoryMapFlags(), &mappedMemory);
    memcpy(mappedMemory, data.data(), size);
    device.logicalDevice->unmapMemory(*stagingMemoryBuffer.memory);
    auto memoryBuffer = createBufferMemory(
            device, size, vk::BufferUsageFlagBits::eTransferDst | usage, vk::MemoryPropertyFlagBits::eDeviceLocal);
    const auto& graphicQueue = device.logicalDevice->getQueue(device.queueFamilyIndices.graphicFamily.value(), 0);
    copyBuffer(device.logicalDevice, commandPool, graphicQueue, stagingMemoryBuffer.buffer, memoryBuffer.buffer, size);
    return memoryBuffer;
}

void transitImageLayout(const Device& device, const vk::UniqueCommandPool& commandPool, const vk::UniqueImage& image,
                        const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout, const uint32_t mipLevels) {
    singleTimeCommand(device, commandPool, [&](const vk::UniqueCommandBuffer& commandBuffer) {
        const auto [barrier, srcPipelineStage, dstPipelineStage] = [&] {
            const auto createBarrier = [&](const vk::AccessFlags srcAccessFlag, const vk::AccessFlags dstAccessFlag) {
                return vk::ImageMemoryBarrier(srcAccessFlag,
                                              dstAccessFlag,
                                              oldLayout,
                                              newLayout,
                                              vk::QueueFamilyIgnored,
                                              vk::QueueFamilyIgnored,
                                              *image,
                                              vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 1));
            };

            if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
                return std::tuple(createBarrier(vk::AccessFlags(), vk::AccessFlagBits::eTransferWrite),
                                  vk::PipelineStageFlagBits::eTopOfPipe,
                                  vk::PipelineStageFlagBits::eTransfer);
            }
            if (oldLayout == vk::ImageLayout::eTransferDstOptimal
                && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
                return std::tuple(createBarrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead),
                                  vk::PipelineStageFlagBits::eTransfer,
                                  vk::PipelineStageFlagBits::eFragmentShader);
            }
            throw std::runtime_error("failed to transit image layout!");
        }();
        commandBuffer->pipelineBarrier(
                srcPipelineStage, dstPipelineStage, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);
    });
}

void copyBufferToImage(const Device& device, const vk::UniqueCommandPool& commandPool, const vk::UniqueBuffer& buffer,
                       const vk::UniqueImage& image, const Resolution& resolution) {
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

[[nodiscard]] auto createImageView(const vk::Device& device, const vk::Image& image, const vk::Format format,
                                   const vk::ImageAspectFlags aspectFlag, uint32_t mipLevels = 1) noexcept
        -> vk::UniqueImageView {
    return device.createImageViewUnique(
            vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(),
                                    image,
                                    vk::ImageViewType::e2D,
                                    format,
                                    vk::ComponentMapping(),
                                    vk::ImageSubresourceRange(aspectFlag, 0, mipLevels, 0, 1)));
}

struct ImageMemory {
    vk::UniqueImage image;
    vk::UniqueDeviceMemory memory;
    vk::UniqueImageView imageView;
};

[[nodiscard]] ImageMemory createImageMemory(const Device& device, const Resolution& resolution, const vk::Format format,
                                            const vk::ImageUsageFlags imageUsageFlags,
                                            const vk::MemoryPropertyFlags memoryPropertyFlags,
                                            const vk::ImageAspectFlags aspectFlags, uint32_t mipLevels = 1) {
    auto image = device.logicalDevice->createImageUnique(
            vk::ImageCreateInfo(vk::ImageCreateFlags(),
                                vk::ImageType::e2D,
                                format,
                                vk::Extent3D(resolution.width, resolution.height, 1),
                                mipLevels,
                                1,
                                vk::SampleCountFlagBits::e1,
                                vk::ImageTiling::eOptimal,
                                imageUsageFlags,
                                vk::SharingMode::eExclusive

                                ));
    vk::MemoryRequirements memoryRequirements;
    device.logicalDevice->getImageMemoryRequirements(*image, &memoryRequirements);
    auto memory = device.logicalDevice->allocateMemoryUnique(vk::MemoryAllocateInfo(
            memoryRequirements.size,
            findMemoryType(device.physicalDevice, memoryRequirements.memoryTypeBits, memoryPropertyFlags)));
    device.logicalDevice->bindImageMemory(*image, *memory, 0);
    auto imageView = createImageView(*device.logicalDevice, *image, format, aspectFlags, mipLevels);
    return ImageMemory{ .image = std::move(image), .memory = std::move(memory), .imageView = std::move(imageView) };
}

void generateMipmaps(const Device& device, const vk::UniqueCommandPool& commandPool, const vk::UniqueImage& image,
                     const vk::Format format, const Resolution& resolution, const uint32_t mipLevels) {

    if (!(device.physicalDevice.getFormatProperties(format).optimalTilingFeatures
          & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("Failed to generate mipmaps! Unsupported linear filtering!");
    }
    singleTimeCommand(device, commandPool, [&](const vk::UniqueCommandBuffer& commandBuffer) {
        const auto getImageBarrier = [&image](const uint32_t mipLevel) -> vk::ImageMemoryBarrier {
            return vk::ImageMemoryBarrier(vk::AccessFlagBits::eTransferWrite,
                                          vk::AccessFlagBits::eTransferRead,
                                          vk::ImageLayout::eTransferDstOptimal,
                                          vk::ImageLayout::eTransferSrcOptimal,
                                          vk::QueueFamilyIgnored,
                                          vk::QueueFamilyIgnored,
                                          *image,
                                          vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, mipLevel, 1, 0, 1));
        };
        const auto getImageBlit =
                [&resolution](const uint32_t mipLevel, const int mipWidth, const int mipHeight) -> vk::ImageBlit {
            return vk::ImageBlit(
                    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, mipLevel, 0, 1),
                    std::array{ vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ mipWidth, mipHeight, 1 } },
                    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, mipLevel + 1, 0, 1),
                    std::array{
                            vk::Offset3D{ 0, 0, 0 },
                            vk::Offset3D{ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 } });
        };
        int mipWidth = static_cast<int>(resolution.width), mipHeight = static_cast<int>(resolution.height);
        for (uint32_t mipLevel = 0; mipLevel < mipLevels - 1; ++mipLevel, mipWidth /= 2, mipHeight /= 2) {
            auto barrier = getImageBarrier(mipLevel);
            commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                           vk::PipelineStageFlagBits::eTransfer,
                                           vk::DependencyFlags(),
                                           {},
                                           {},
                                           { barrier });

            const auto blit = getImageBlit(mipLevel, mipWidth, mipHeight);
            commandBuffer->blitImage(*image,
                                     vk::ImageLayout::eTransferSrcOptimal,
                                     *image,
                                     vk::ImageLayout::eTransferDstOptimal,
                                     { blit },
                                     vk::Filter::eLinear);
            barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                           vk::PipelineStageFlagBits::eFragmentShader,
                                           vk::DependencyFlags(),
                                           {},
                                           {},
                                           { barrier });
        }
        auto barrier = getImageBarrier(mipLevels - 1);
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                       vk::PipelineStageFlagBits::eFragmentShader,
                                       vk::DependencyFlags(),
                                       {},
                                       {},
                                       { barrier });
    });
}

[[nodiscard]] ImageMemory createImageMemory(const Device& device, const vk::UniqueCommandPool& commandPool,
                                            const std::span<const uint8_t> data, const Resolution& resolution,
                                            const uint32_t mipLevels = 1) {
    const auto stagingMemoryBuffer =
            createBufferMemory(device,
                               data.size(),
                               vk::BufferUsageFlagBits::eTransferSrc,
                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    void* mappedMemory = nullptr;
    [[maybe_unused]] const auto result = device.logicalDevice->mapMemory(
            *stagingMemoryBuffer.memory, 0, data.size(), vk::MemoryMapFlags(), &mappedMemory);
    memcpy(mappedMemory, data.data(), data.size());
    device.logicalDevice->unmapMemory(*stagingMemoryBuffer.memory);

    auto imageMemory = createImageMemory(device,
                                         resolution,
                                         vk::Format::eR8G8B8A8Srgb,
                                         vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
                                                 | vk::ImageUsageFlagBits::eSampled,
                                         vk::MemoryPropertyFlagBits::eDeviceLocal,
                                         vk::ImageAspectFlagBits::eColor,
                                         mipLevels);

    transitImageLayout(device,
                       commandPool,
                       imageMemory.image,
                       vk::ImageLayout::eUndefined,
                       vk::ImageLayout::eTransferDstOptimal,
                       mipLevels);
    copyBufferToImage(device, commandPool, stagingMemoryBuffer.buffer, imageMemory.image, resolution);
    generateMipmaps(device, commandPool, imageMemory.image, vk::Format::eR8G8B8A8Srgb, resolution, mipLevels);
    /*transitImageLayout(device,
                       commandPool,
                       imageMemory.image,
                       vk::ImageLayout::eTransferDstOptimal,
                       vk::ImageLayout::eShaderReadOnlyOptimal);*/

    return imageMemory;
}

[[nodiscard]] auto createImageSampler(const Device& device, const uint32_t mipLevels) noexcept -> vk::UniqueSampler {
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

[[nodiscard]] auto findSupportedImageFormat(const vk::PhysicalDevice& device, const std::span<const vk::Format> formats,
                                            const vk::ImageTiling imageTiling, const vk::FormatFeatureFlags features)
        -> vk::Format {
    for (const auto format : formats) {
        const auto properties = device.getFormatProperties(format);
        if (imageTiling == vk::ImageTiling::eLinear && (properties.linearTilingFeatures & features) == features) {
            return format;
        }
        if (imageTiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("filed to find image format");
}

[[nodiscard]] auto findDepthFormat(const vk::PhysicalDevice& device) -> vk::Format {
    constexpr auto formats =
            std::array{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint };
    return findSupportedImageFormat(device,
                                    std::span(formats.data(), formats.size()),
                                    vk::ImageTiling::eOptimal,
                                    vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

[[nodiscard]] bool hasStencilFormat(const vk::Format format) noexcept {
    return format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD32SfloatS8Uint;
}

}// namespace Thyme::Vulkan
