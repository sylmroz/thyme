module;

#include <functional>
#include <vector>

#include <fmt/format.h>
#include <vulkan/vulkan.hpp>

export module thyme.platform.vulkan_renderer;
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
    explicit QueueFamilyIndices(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface);

    std::optional<uint32_t> graphicFamily;
    std::optional<uint32_t> presentFamily;

    [[nodiscard]] constexpr bool isCompleted() const noexcept {
        return graphicFamily.has_value() && presentFamily.has_value();
    }
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

    [[nodiscard]] inline auto getBestSurfaceFormat() const noexcept {
        const auto suitableFormat = std::ranges::find_if(formats, [](const vk::SurfaceFormatKHR& format) {
            return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
        if (suitableFormat != formats.end()) {
            return *suitableFormat;
        }
        return formats[0];
    }

    [[nodiscard]] inline auto getBestPresetMode() const noexcept {
        const auto suitablePreset = std::ranges::find_if(presentModes, [](const vk::PresentModeKHR& presentMode) {
            return presentMode == vk::PresentModeKHR::eMailbox;
        });

        if (suitablePreset != presentModes.end()) {
            return *suitablePreset;
        }
        return vk::PresentModeKHR::eFifo;
    }

    [[nodiscard]] inline auto getSwapExtent(const Resolution& fallbackResolution) const noexcept {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        const auto [width, height] = fallbackResolution;
        const auto actualExtent = vk::Extent2D{
            std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        };
        return actualExtent;
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

std::vector<PhysicalDevice> getPhysicalDevices(const vk::UniqueInstance& instance, const vk::UniqueSurfaceKHR& surface);

struct SwapChainDetails {
    vk::SurfaceFormatKHR surfaceFormat;
    vk::PresentModeKHR presetMode;
    vk::Extent2D extent;
};

class SwapChain {
public:
    explicit SwapChain(const SwapChainDetails& swapChainDetails,
                       const PhysicalDevice& device,
                       const vk::UniqueDevice& logicalDevice,
                       const vk::UniqueSurfaceKHR& surface);

    vk::UniqueSwapchainKHR swapChain;
    std::vector<vk::Image> images;
    std::vector<vk::UniqueImageView> imageViews;

private:
    SwapChainDetails m_swapChainDetails;
};

}// namespace Thyme::Vulkan
