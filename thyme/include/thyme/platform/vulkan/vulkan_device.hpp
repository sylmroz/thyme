#pragma once

#include <vector>

#include <thyme/platform/vulkan/utils.hpp>

namespace th::vulkan {

struct VulkanDevice {
    vk::PhysicalDevice physicalDevice;
    vk::Device logicalDevice;
    vk::CommandPool commandPool;
    QueueFamilyIndices queueFamilyIndices;
    vk::SampleCountFlagBits maxMsaaSamples;

    auto getGraphicQueue() const noexcept -> vk::Queue {
        return logicalDevice.getQueue(queueFamilyIndices.graphicFamily.value(), 0);
    }

    auto getPresentationQueue() const noexcept -> vk::Queue {
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
        vulkan::singleTimeCommand(commandBuffer.get(), graphicQueue, fun, args...);
    }
};

class PhysicalDevicesManager {
    class PhysicalDevice {
    public:
        explicit PhysicalDevice(const vk::PhysicalDevice physicalDevice, const QueueFamilyIndices& queueFamilyIndices,
                                const vk::SampleCountFlagBits maxMsaaSamples) noexcept
            : physicalDevice{ physicalDevice }, queueFamilyIndices{ queueFamilyIndices },
              maxMsaaSamples{ maxMsaaSamples } {}

        vk::PhysicalDevice physicalDevice;
        QueueFamilyIndices queueFamilyIndices;
        vk::SampleCountFlagBits maxMsaaSamples;

        [[nodiscard]] vk::UniqueDevice createLogicalDevice() const;
    };

    struct VulkanInternalDevice {
        explicit VulkanInternalDevice(const PhysicalDevice& physicalDevice)
            : physicalDevice(physicalDevice.physicalDevice), logicalDevice(physicalDevice.createLogicalDevice()),
              queueFamilyIndices(physicalDevice.queueFamilyIndices), maxMsaaSamples{ physicalDevice.maxMsaaSamples },
              commandPool{ logicalDevice->createCommandPoolUnique(
                      vk::CommandPoolCreateInfo{ .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                                 .queueFamilyIndex = queueFamilyIndices.graphicFamily.value() }) } {}
        vk::PhysicalDevice physicalDevice;
        vk::UniqueDevice logicalDevice;
        QueueFamilyIndices queueFamilyIndices;
        vk::SampleCountFlagBits maxMsaaSamples;
        vk::UniqueCommandPool commandPool;

        auto getGraphicQueue() const noexcept -> vk::Queue {
            return logicalDevice->getQueue(queueFamilyIndices.graphicFamily.value(), 0);
        }

        auto getPresentationQueue() const noexcept -> vk::Queue {
            return logicalDevice->getQueue(queueFamilyIndices.presentFamily.value(), 0);
        }
    };

public:
    explicit PhysicalDevicesManager(vk::Instance instance, vk::SurfaceKHR surface);

    [[nodiscard]] auto getSelectedDevice() const noexcept -> VulkanDevice {
        return VulkanDevice{
            .physicalDevice = m_selectedDevice.physicalDevice,
            .logicalDevice = m_selectedDevice.logicalDevice.get(),
            .commandPool = m_selectedDevice.commandPool.get(),
            .queueFamilyIndices = m_selectedDevice.queueFamilyIndices,
            .maxMsaaSamples = m_selectedDevice.maxMsaaSamples,
        };
    }

    void selectDevice(uint32_t index) {
        if (index - 1 >= m_physicalDevices.size()) {
            const auto message =
                    fmt::format("Selecting physical device failed! Selected index is {}, but devices are {}",
                                index,
                                m_physicalDevices.size());
            throw std::runtime_error(message);
        }
        m_selectedDevice = VulkanInternalDevice(*(m_physicalDevices.begin() + index - 1));
    }

private:
    auto getPhysicalDevices(vk::Instance instance, vk::SurfaceKHR surface) const -> std::vector<PhysicalDevice>;

private:
    std::vector<PhysicalDevice> m_physicalDevices{};
    VulkanInternalDevice m_selectedDevice;
};

}// namespace th::vulkan
