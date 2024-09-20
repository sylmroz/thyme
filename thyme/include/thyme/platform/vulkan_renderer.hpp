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

// TODO - the class should support more queue family flags like eSparseBinding
struct QueueFamilyIndices {
    explicit QueueFamilyIndices(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface);

    std::optional<uint32_t> graphicFammily;
    std::optional<uint32_t> presentFamily;

    [[nodiscard]] constexpr bool isCompleted() const noexcept {
        return graphicFammily.has_value() && presentFamily.has_value();
    }
};

class PhysicalDevice {
public:
    explicit PhysicalDevice(const vk::PhysicalDevice& physicalDevice,
                            const QueueFamilyIndices& queueFamilyIndices) noexcept
        : physicalDevice{ physicalDevice }, queueFamilyIndices{ queueFamilyIndices } {
        logicalDevice = createLogicalDevice();
    }

    QueueFamilyIndices queueFamilyIndices;
    vk::PhysicalDevice physicalDevice;
    vk::Device logicalDevice;

private:
    vk::Device createLogicalDevice();
};

std::vector<PhysicalDevice> getPhysicalDevices(const vk::UniqueInstance& instance, const vk::UniqueSurfaceKHR& surface);

// TODO - read from config, last selected device
class PhysicalDevicesManager {
public:
    PhysicalDevicesManager(const std::vector<PhysicalDevice>& devices) {
        m_physicalDevices = devices;
        m_selectedDevice = m_physicalDevices.begin();
    }

    [[nodiscard]] const auto& getSelectedDevice() const noexcept {
        return *m_selectedDevice;
    }

    [[nodiscard]] const auto& getDevicesList() const noexcept {
        return m_physicalDevices;
    }

    void selectDevice(uint32_t index) {
        if (index - 1 >= m_physicalDevices.size()) {
            const auto message =
                    fmt::format("Selecting physical device failed! Selected index is {}, but devices are {}",
                                index,
                                m_physicalDevices.size());
            TH_API_LOG_ERROR(message);
            throw std::runtime_error(message.c_str());
        }
        m_selectedDevice = m_physicalDevices.begin() + index - 1;
    }

private:
    std::vector<PhysicalDevice> m_physicalDevices;
    std::vector<PhysicalDevice>::iterator m_selectedDevice;
};

}// namespace Thyme::Vulkan
