module;

#include <numeric>
#include <vector>

#include <fmt/format.h>
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

[[nodiscard]] vk::UniqueDescriptorPool createDescriptorPool(const vk::UniqueDevice& device, const std::vector<vk::DescriptorPoolSize>& descriptorSizes)
{
    const uint32_t maxSet = std::accumulate(std::begin(descriptorSizes), std::end(descriptorSizes), 0,
            [](const uint32_t sum, const vk::DescriptorPoolSize& descriptorPoolSize) { return sum + descriptorPoolSize.descriptorCount; });

    return device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, maxSet, static_cast<uint32_t>(descriptorSizes.size()), descriptorSizes.data()));
}
// TODO - can it be done much better??
template <typename F, typename... Args>
        void singleTimeCommand(const vk::UniqueCommandBuffer& commandBuffer, const vk::UniqueCommandPool& commandPool, const vk::Queue& graphicQueue, F fun, Args... args)
{
    commandBuffer->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    fun(commandBuffer, args...);
    commandBuffer->end();
    graphicQueue.submit(vk::SubmitInfo(0, nullptr, nullptr, 1, &(commandBuffer.get())), nullptr);
    graphicQueue.waitIdle();
}

template <typename F, typename... Args>
void singleTimeCommand(const vk::UniqueDevice& device, const vk::UniqueCommandPool& commandPool, const vk::Queue& graphicQueue, F fun, Args... args)
{
    auto commandBuffer = std::move(
            device->allocateCommandBuffersUnique(
                    vk::CommandBufferAllocateInfo(commandPool.get(), vk::CommandBufferLevel::ePrimary, 1)).front());
    singleTimeCommand(commandBuffer, commandPool, graphicQueue, fun, args...);
}

}// namespace Thyme::Vulkan
