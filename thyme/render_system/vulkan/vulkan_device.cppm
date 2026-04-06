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

class PhysicalDevice {
public:
    explicit PhysicalDevice(const vk::raii::PhysicalDevice& physical_device,
                            const QueueFamilyIndices& queue_family_indices,
                            const vk::SampleCountFlagBits max_msaa_samples) noexcept
        : physical_device{ physical_device }, queue_family_indices{ queue_family_indices },
          max_msaa_samples{ max_msaa_samples } {}

    vk::raii::PhysicalDevice physical_device;
    QueueFamilyIndices queue_family_indices;
    vk::SampleCountFlagBits max_msaa_samples;

    [[nodiscard]] auto createLogicalDevice() const -> vk::raii::Device;
};

export struct VulkanDevice {
    explicit VulkanDevice(const PhysicalDevice& physical_device)
        : physical_device(physical_device.physical_device), logical_device(physical_device.createLogicalDevice()),
          queue_family_indices(physical_device.queue_family_indices),
          max_msaa_samples{ physical_device.max_msaa_samples },
          command_pool{ logical_device,
                        vk::CommandPoolCreateInfo{ .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                                   .queueFamilyIndex = queue_family_indices.graphic_family.value() } } {
    }

    [[nodiscard]] auto getLogicalDevice() const noexcept -> const vk::raii::Device& {
        return logical_device;
    }

    vk::raii::PhysicalDevice physical_device;
    vk::raii::Device logical_device;
    QueueFamilyIndices queue_family_indices;
    vk::SampleCountFlagBits max_msaa_samples;
    vk::raii::CommandPool command_pool;

    [[nodiscard]] auto getGraphicQueue() const noexcept -> vk::Queue {
        return logical_device.getQueue(queue_family_indices.graphic_family.value(), 0);
    }

    [[nodiscard]] auto getPresentationQueue() const noexcept -> vk::Queue {
        return logical_device.getQueue(queue_family_indices.present_family.value(), 0);
    }

    template <typename F, typename... Args>
        requires(InvocableCommandWithCommandBuffer<F, Args...>)
    void singleTimeCommand(F fun, Args... args) const {
        const auto commandBuffer = std::move(logical_device
                                                     .allocateCommandBuffers(vk::CommandBufferAllocateInfo{
                                                             .commandPool = command_pool,
                                                             .level = vk::CommandBufferLevel::ePrimary,
                                                             .commandBufferCount = 1 })
                                                     .front());
        const auto graphicQueue = getGraphicQueue();
        th::singleTimeCommand(commandBuffer, graphicQueue, fun, args...);
    }
};

export class VulkanPhysicalDevicesManager {
public:
    explicit VulkanPhysicalDevicesManager(const std::span<const vk::raii::PhysicalDevice> physical_devices,
                                          const std::optional<vk::SurfaceKHR> surface, Logger& logger)
        : m_physical_devices{ enumeratePhysicalDevices(physical_devices, surface.value()) },
          m_selected_device{ VulkanDevice{ m_physical_devices.front() } }, m_logger{ logger } {}

    [[nodiscard]] auto getCurrentDevice() const noexcept -> const VulkanDevice& {
        return m_selected_device;
    }

    void selectDevice(uint32_t index) {
        if (index - 1 >= m_physical_devices.size()) {
            const auto message =
                    std::format("Selecting physical device failed! Selected index is {}, but devices are {}",
                                index,
                                m_physical_devices.size());
            m_logger.error("{}", message);
            throw std::runtime_error(message);
        }
        m_selected_device = VulkanDevice(*(m_physical_devices.begin() + index - 1));
    }

private:
    [[nodiscard]] auto enumeratePhysicalDevices(std::span<const vk::raii::PhysicalDevice> physical_devices,
                                                vk::SurfaceKHR surface) const -> std::vector<PhysicalDevice>;

private:
    std::vector<PhysicalDevice> m_physical_devices{};
    VulkanDevice m_selected_device;

    Logger& m_logger;
};

}// namespace th
