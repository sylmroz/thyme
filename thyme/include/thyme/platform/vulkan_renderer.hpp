#pragma once

#include "thyme/core/logger.hpp"
#include "thyme/version.hpp"

#include <string_view>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace Thyme::Vulkan {

struct UniqueInstanceConfig {
    std::string_view engineName;
    std::string_view appName;
    std::vector<const char*> instanceLayers;
    std::vector<const char*> instanceExtension;
};

class UniqueInstance {
public:
    UniqueInstance(const UniqueInstanceConfig& config);
    vk::UniqueInstance instance;
};

struct QueueFamilyIndices {
    QueueFamilyIndices(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface) {
        auto queueFamilies = device.getQueueFamilyProperties2();
        uint32_t i{ 0 };

        for (const auto& queueFamily : queueFamilies) {
            const auto& queueFamilyProperties = queueFamily.queueFamilyProperties;
            if (queueFamilyProperties.queueCount <= 0) {
                ++i;
                continue;
            }
            if (queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) {
                graphicFammily = i;
            }
            if (device.getSurfaceSupportKHR(i, surface)) {
                presentFamily = i;
            }
            if (isCompleted()) {
                break;
            }
        }
    }
    std::optional<uint32_t> graphicFammily;
    std::optional<uint32_t> presentFamily;

    [[nodiscard]] constexpr bool isCompleted() const noexcept {
        return graphicFammily.has_value() && presentFamily.has_value();
    }
};

class PhysicalDevice {
public:
private:

};

}// namespace Thyme::Vulkan
