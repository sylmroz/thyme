#pragma once

#include <fmt/core.h>
#include <vector>

#include <thyme/platform/vulkan/utils.hpp>

namespace th::vulkan {

// TODO - read from config the last selected device
class PhysicalDevicesManager {
public:
    explicit PhysicalDevicesManager(const std::vector<PhysicalDevice>& devices) : m_physicalDevices(devices) {
        m_selectedDevice = m_physicalDevices.begin();
    }

    [[nodiscard]] auto getSelectedDevice(this const PhysicalDevicesManager& self) noexcept -> Device {
        return Device(*self.m_selectedDevice);
    }

    [[nodiscard]] auto getDevicesList(this const PhysicalDevicesManager& self) noexcept -> std::vector<PhysicalDevice> {
        return self.m_physicalDevices;
    }

    void selectDevice(uint32_t index) {
        if (index - 1 >= m_physicalDevices.size()) {
            const auto message =
                    fmt::format("Selecting physical device failed! Selected index is {}, but devices are {}",
                                index,
                                m_physicalDevices.size());
            throw std::runtime_error(message);
        }
        m_selectedDevice = m_physicalDevices.begin() + index - 1;
    }

private:
    std::vector<PhysicalDevice> m_physicalDevices;
    decltype(m_physicalDevices)::iterator m_selectedDevice;
};

}// namespace th::vulkan
