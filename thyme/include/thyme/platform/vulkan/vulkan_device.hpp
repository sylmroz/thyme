#pragma once

#include <vector>

#include <thyme/platform/vulkan/utils.hpp>

namespace th::vulkan {

struct VulkanDevice {
    vk::PhysicalDevice physicalDevice;
    vk::Device logicalDevice;
    vk::CommandPool commandPool;
    QueueFamilyIndices queueFamilyIndices;
    SwapChainSupportDetails swapChainSupportDetails;
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
                                                     .allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
                                                             commandPool, vk::CommandBufferLevel::ePrimary, 1))
                                                     .front());
        const auto graphicQueue = getGraphicQueue();
        vulkan::singleTimeCommand(commandBuffer.get(), graphicQueue, fun, args...);
    }
};

class PhysicalDevicesManager {
    class PhysicalDevice {
    public:
        explicit PhysicalDevice(const vk::PhysicalDevice& physicalDevice, const QueueFamilyIndices& queueFamilyIndices,
                                const SwapChainSupportDetails& swapChainSupportDetails,
                                const vk::SampleCountFlagBits maxMsaaSamples) noexcept
            : physicalDevice{ physicalDevice }, queueFamilyIndices{ queueFamilyIndices },
              swapChainSupportDetails{ swapChainSupportDetails }, maxMsaaSamples{ maxMsaaSamples } {}

        vk::PhysicalDevice physicalDevice;
        QueueFamilyIndices queueFamilyIndices;
        SwapChainSupportDetails swapChainSupportDetails;
        vk::SampleCountFlagBits maxMsaaSamples;

        [[nodiscard]] vk::UniqueDevice createLogicalDevice() const;
    };

    struct VulkanInternalDevice {
        explicit VulkanInternalDevice(PhysicalDevice physicalDevice)
            : physicalDevice(physicalDevice.physicalDevice), logicalDevice(physicalDevice.createLogicalDevice()),
              queueFamilyIndices(physicalDevice.queueFamilyIndices),
              swapChainSupportDetails(physicalDevice.swapChainSupportDetails),
              maxMsaaSamples{ physicalDevice.maxMsaaSamples },
              commandPool{ logicalDevice->createCommandPoolUnique(
                      vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                                queueFamilyIndices.graphicFamily.value())) } {}
        vk::PhysicalDevice physicalDevice;
        vk::UniqueDevice logicalDevice;
        QueueFamilyIndices queueFamilyIndices;
        SwapChainSupportDetails swapChainSupportDetails;
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
            .swapChainSupportDetails = m_selectedDevice.swapChainSupportDetails,
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
    auto getPhysicalDevices(vk::Instance instance, vk::SurfaceKHR surface) -> std::vector<PhysicalDevice>;

private:
    std::vector<PhysicalDevice> m_physicalDevices{};
    VulkanInternalDevice m_selectedDevice;
};

}// namespace th::vulkan
