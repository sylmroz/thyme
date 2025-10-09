export module th.render_system.vulkan:device;

import std;

import vulkan_hpp;

import th.core.logger;

import :utils;

namespace th {

export struct VulkanDevice {
    vk::PhysicalDevice physical_device;
    vk::Device logical_device;
    vk::CommandPool command_pool;
    QueueFamilyIndices queue_family_indices;
    vk::SampleCountFlagBits max_msaa_samples;

    [[nodiscard]] auto getGraphicQueue() const noexcept -> vk::Queue {
        return logical_device.getQueue(queue_family_indices.graphic_family.value(), 0);
    }

    [[nodiscard]] auto getPresentationQueue() const noexcept -> vk::Queue {
        return logical_device.getQueue(queue_family_indices.present_family.value(), 0);
    }

    template <typename F, typename... Args>
        requires(InvocableCommandWithCommandBuffer<F, Args...>)
    void singleTimeCommand(F fun, Args... args) {
        const auto commandBuffer = std::move(logical_device
                                                     .allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{
                                                             .commandPool = command_pool,
                                                             .level = vk::CommandBufferLevel::ePrimary,
                                                             .commandBufferCount = 1 })
                                                     .front());
        const auto graphicQueue = getGraphicQueue();
        th::singleTimeCommand(commandBuffer.get(), graphicQueue, fun, args...);
    }
};

export class VulkanPhysicalDevicesManager {
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

    struct InternalDevice {
        explicit InternalDevice(const PhysicalDevice& physical_device)
            : physical_device(physical_device.physical_device), logical_device(physical_device.createLogicalDevice()),
              queue_family_indices(physical_device.queue_family_indices),
              max_msaa_samples{ physical_device.max_msaa_samples },
              command_pool{ logical_device,
                            vk::CommandPoolCreateInfo{ .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                                       .queueFamilyIndex =
                                                               queue_family_indices.graphic_family.value() } } {}
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
    };

public:
    explicit VulkanPhysicalDevicesManager(const vk::raii::Instance& instance,
                                          const std::optional<vk::SurfaceKHR> surface, Logger& logger)
        : m_physical_devices{ enumeratePhysicalDevices(instance, surface) },
          m_selected_device{ InternalDevice{ m_physical_devices.front() } }, m_logger{ logger } {}

    [[nodiscard]] auto getSelectedDevice() const noexcept -> VulkanDevice {
        return VulkanDevice{
            .physical_device = m_selected_device.physical_device,
            .logical_device = *m_selected_device.logical_device,
            .command_pool = *m_selected_device.command_pool,
            .queue_family_indices = m_selected_device.queue_family_indices,
            .max_msaa_samples = m_selected_device.max_msaa_samples,
        };
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
        m_selected_device = InternalDevice(*(m_physical_devices.begin() + index - 1));
    }

private:
    [[nodiscard]] auto enumeratePhysicalDevices(const vk::raii::Instance& instance,
                                                std::optional<vk::SurfaceKHR> surface) const
            -> std::vector<PhysicalDevice>;

private:
    std::vector<PhysicalDevice> m_physical_devices{};
    InternalDevice m_selected_device;

    Logger& m_logger;
};

}// namespace th
