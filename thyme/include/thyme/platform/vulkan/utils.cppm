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
                           const vk::UniqueSurfaceKHR& surface, const vk::SwapchainKHR& oldSwapChain = {});

    vk::UniqueSwapchainKHR swapChain;
    std::vector<SwapChainFrame> swapChainFrame;
};

struct FrameData {
    vk::UniqueCommandBuffer commandBuffer;
    vk::UniqueSemaphore imageAvailableSemaphore;
    vk::UniqueSemaphore renderFinishedSemaphore;
    vk::UniqueFence fence;
};

[[nodiscard]] inline auto createFrameDataList(const vk::UniqueDevice& logicalDevice,
                                              const vk::UniqueCommandPool& commandPool,
                                              const uint32_t maxFrames) noexcept -> std::vector<FrameData> {
    std::vector<FrameData> frameDataList;
    frameDataList.reserve(maxFrames);
    for (int i = 0; i < maxFrames; i++) {
        frameDataList.emplace_back(FrameData{
                .commandBuffer = std::move(logicalDevice
                                                   ->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
                                                           *commandPool, vk::CommandBufferLevel::ePrimary, 1))
                                                   .front()),
                .imageAvailableSemaphore = logicalDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo()),
                .renderFinishedSemaphore = logicalDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo()),
                .fence = logicalDevice->createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)) });
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

[[nodiscard]] auto createRenderPass(const vk::UniqueDevice& logicalDevice, const vk::Format format)
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

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;

    static constexpr auto getBindingDescription() -> vk::VertexInputBindingDescription {
        return vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
    }

    static constexpr auto getAttributeDescriptions() -> std::array<vk::VertexInputAttributeDescription, 2> {
        return std::array{
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
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


struct MemoryBuffer {
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
};

[[nodiscard]] MemoryBuffer createMemoryBuffer(const Device& device, const uint32_t size,
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

    return MemoryBuffer{ .buffer = std::move(buffer), .memory = std::move(memory) };
}

template <typename Vec>
[[nodiscard]] MemoryBuffer createMemoryBuffer(const Device& device, const vk::UniqueCommandPool& commandPool,
                                              const Vec& data, const vk::BufferUsageFlags usage) {
    const auto size = data.size() * sizeof(data[0]);

    const auto stagingMemoryBuffer =
            createMemoryBuffer(device,
                               size,
                               vk::BufferUsageFlagBits::eTransferSrc,
                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* mappedMemory = nullptr;
    [[maybe_unused]] const auto result =
            device.logicalDevice->mapMemory(*stagingMemoryBuffer.memory, 0, size, vk::MemoryMapFlags(), &mappedMemory);
    memcpy(mappedMemory, data.data(), size);
    device.logicalDevice->unmapMemory(*stagingMemoryBuffer.memory);
    auto memoryBuffer = createMemoryBuffer(
            device, size, vk::BufferUsageFlagBits::eTransferDst | usage, vk::MemoryPropertyFlagBits::eDeviceLocal);
    const auto& graphicQueue = device.logicalDevice->getQueue(device.queueFamilyIndices.graphicFamily.value(), 0);
    copyBuffer(device.logicalDevice, commandPool, graphicQueue, stagingMemoryBuffer.buffer, memoryBuffer.buffer, size);
    return memoryBuffer;
}
}// namespace Thyme::Vulkan
