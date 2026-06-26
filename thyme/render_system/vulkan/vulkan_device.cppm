export module th.render_system.vulkan:device;

import std;

import vulkan;

import th.core.logger;

import :utils;

namespace th {

export [[nodiscard]] auto getMaxUsableSampleCount(vk::PhysicalDevice device) noexcept -> vk::SampleCountFlagBits;
export [[nodiscard]] auto filterDevices(std::span<const vk::raii::PhysicalDevice> physical_devices,
                                        vk::SurfaceKHR surface) -> std::vector<vk::raii::PhysicalDevice>;

export [[nodiscard]] auto createLogicalDevice(const vk::raii::PhysicalDevice& physical_device, uint32_t queue_index)
        -> vk::raii::Device;

export class PhysicalDevice2 {
public:
    explicit PhysicalDevice2(const vk::raii::PhysicalDevice& physical_device);

    vk::raii::PhysicalDevice physical_device;
    vk::SampleCountFlagBits max_msaa_samples;
    std::string device_name;
};

export class PhysicalDevices2 {
    using iter = std::vector<PhysicalDevice2>::iterator;

public:
    explicit PhysicalDevices2(std::span<const vk::raii::PhysicalDevice> physical_devices);

    [[nodiscard]] auto begin() noexcept -> iter {
        return physical_devices.begin();
    }
    [[nodiscard]] auto end() noexcept -> iter {
        return physical_devices.end();
    }

    [[nodiscard]] auto front() noexcept -> PhysicalDevice2& { return physical_devices.front();}

    [[nodiscard]] auto current() noexcept -> vk::raii::PhysicalDevice& {
        return front().physical_device;
    }

    std::vector<PhysicalDevice2> physical_devices;

private:
    static [[nodiscard]] auto sortPhysicalDevices(std::span<const vk::raii::PhysicalDevice> physical_devices)
            -> std::vector<PhysicalDevice2>;
};

}// namespace th
