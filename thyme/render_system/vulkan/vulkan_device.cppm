module;

#include <vector>

export module th.render_system.vulkan.device;

import th.render_system.vulkan.utils;

import vulkan_hpp;

namespace th {

export struct VulkanDevice {
    vk::PhysicalDevice physicalDevice;
    vk::Device logicalDevice;
    vk::CommandPool commandPool;
    QueueFamilyIndices queueFamilyIndices;
    vk::SampleCountFlagBits maxMsaaSamples;

    [[nodiscard]] auto getGraphicQueue() const noexcept -> vk::Queue {
        return logicalDevice.getQueue(queueFamilyIndices.graphicFamily.value(), 0);
    }

    [[nodiscard]] auto getPresentationQueue() const noexcept -> vk::Queue {
        return logicalDevice.getQueue(queueFamilyIndices.presentFamily.value(), 0);
    }

    template <typename F, typename... Args>
        requires(InvocableCommandWithCommandBuffer<F, Args...>)
    void singleTimeCommand(F fun, Args... args) {
        const auto commandBuffer = std::move(logicalDevice
                                                     .allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{
                                                             .commandPool = commandPool,
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
        explicit PhysicalDevice(const vk::raii::PhysicalDevice& physicalDevice,
                                const QueueFamilyIndices& queueFamilyIndices,
                                const vk::SampleCountFlagBits maxMsaaSamples) noexcept
            : physicalDevice{ physicalDevice }, queueFamilyIndices{ queueFamilyIndices },
              maxMsaaSamples{ maxMsaaSamples } {}

        vk::raii::PhysicalDevice physicalDevice;
        QueueFamilyIndices queueFamilyIndices;
        vk::SampleCountFlagBits maxMsaaSamples;

        [[nodiscard]] auto createLogicalDevice() const -> vk::raii::Device;
    };

    struct InternalDevice {
        explicit InternalDevice(const PhysicalDevice& physicalDevice)
            : physicalDevice(physicalDevice.physicalDevice), logicalDevice(physicalDevice.createLogicalDevice()),
              queueFamilyIndices(physicalDevice.queueFamilyIndices), maxMsaaSamples{ physicalDevice.maxMsaaSamples },
              commandPool{ logicalDevice,
                           vk::CommandPoolCreateInfo{ .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                                      .queueFamilyIndex = queueFamilyIndices.graphicFamily.value() } } {
        }
        vk::raii::PhysicalDevice physicalDevice;
        vk::raii::Device logicalDevice;
        QueueFamilyIndices queueFamilyIndices;
        vk::SampleCountFlagBits maxMsaaSamples;
        vk::raii::CommandPool commandPool;

        [[nodiscard]] auto getGraphicQueue() const noexcept -> vk::Queue {
            return logicalDevice.getQueue(queueFamilyIndices.graphicFamily.value(), 0);
        }

        [[nodiscard]] auto getPresentationQueue() const noexcept -> vk::Queue {
            return logicalDevice.getQueue(queueFamilyIndices.presentFamily.value(), 0);
        }
    };

public:
    explicit VulkanPhysicalDevicesManager(const vk::raii::Instance& instance,
                                          const std::optional<vk::SurfaceKHR>
                                                  surface)
        : m_physicalDevices{ enumeratePhysicalDevices(instance, surface) },
          m_selectedDevice{ InternalDevice{ m_physicalDevices.front() } } {}

    [[nodiscard]] auto getSelectedDevice() const noexcept -> VulkanDevice {
        return VulkanDevice{
            .physicalDevice = m_selectedDevice.physicalDevice,
            .logicalDevice = *m_selectedDevice.logicalDevice,
            .commandPool = *m_selectedDevice.commandPool,
            .queueFamilyIndices = m_selectedDevice.queueFamilyIndices,
            .maxMsaaSamples = m_selectedDevice.maxMsaaSamples,
        };
    }

    void selectDevice(uint32_t index) {
        if (index - 1 >= m_physicalDevices.size()) {
            const auto message =
                    std::format("Selecting physical device failed! Selected index is {}, but devices are {}",
                                index,
                                m_physicalDevices.size());
            throw std::runtime_error(message);
        }
        m_selectedDevice = InternalDevice(*(m_physicalDevices.begin() + index - 1));
    }

private:
    [[nodiscard]] auto enumeratePhysicalDevices(const vk::raii::Instance& instance,
                                                std::optional<vk::SurfaceKHR> surface) const
            -> std::vector<PhysicalDevice>;

private:
    std::vector<PhysicalDevice> m_physicalDevices{};
    InternalDevice m_selectedDevice;
};

}// namespace th
