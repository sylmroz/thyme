module;

#include <functional>
#include <vector>

#include <fmt/format.h>
#include <vulkan/vulkan.hpp>

export module thyme.platform.vulkan:utils;
import thyme.core.common_structs;

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
    vk::Extent2D extent;
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

    [[nodiscard]] inline auto getBestSwapChainSettings(const Resolution& fallbackResolution) const noexcept
            -> SwapChainSettings {
        return SwapChainSettings{ .surfaceFormat = getBestSurfaceFormat(),
                                  .presetMode = getBestPresetMode(),
                                  .extent = getSwapExtent(fallbackResolution) };
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

class SwapChainData {
public:
    explicit SwapChainData(const SwapChainSettings& swapChainDetails,
                           const Device& device,
                           const vk::UniqueRenderPass& renderPass,
                           const vk::UniqueSurfaceKHR& surface,
                           const vk::SwapchainKHR& oldSwapChain = {});

    vk::UniqueSwapchainKHR swapChain;
    std::vector<vk::Image> images;
    std::vector<vk::UniqueImageView> imageViews;
    std::vector<vk::UniqueFramebuffer> frameBuffers;

private:
    SwapChainSettings m_swapChainDetails;
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

}// namespace Thyme::Vulkan
