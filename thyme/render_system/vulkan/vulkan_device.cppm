export module th.render_system.vulkan.device;

#include <thyme/platform/vulkan/utils.hpp>

namespace th::render_system::vulkan {

export struct Device {
    vk::PhysicalDevice physicalDevice;
    vk::Device logicalDevice;
    vk::CommandPool commandPool;
    th::vulkan::QueueFamilyIndices queueFamilyIndices;
    vk::SampleCountFlagBits maxMsaaSamples;

    [[nodiscard]] auto getGraphicQueue() const noexcept -> vk::Queue {
        return logicalDevice.getQueue(queueFamilyIndices.graphicFamily.value(), 0);
    }

    [[nodiscard]] auto getPresentationQueue() const noexcept -> vk::Queue {
        return logicalDevice.getQueue(queueFamilyIndices.presentFamily.value(), 0);
    }

    template <typename F, typename... Args>
        requires(th::vulkan::InvocableCommandWithCommandBuffer<F, Args...>)
    void singleTimeCommand(F fun, Args... args) {
        const auto commandBuffer = std::move(logicalDevice
                                                     .allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{
                                                             .commandPool = commandPool,
                                                             .level = vk::CommandBufferLevel::ePrimary,
                                                             .commandBufferCount = 1 })
                                                     .front());
        const auto graphicQueue = getGraphicQueue();
        th::vulkan::singleTimeCommand(commandBuffer.get(), graphicQueue, fun, args...);
    }
};

export class PhysicalDevicesManager {
    class PhysicalDevice {
    public:
        explicit PhysicalDevice(const vk::PhysicalDevice physicalDevice,
                                const th::vulkan::QueueFamilyIndices& queueFamilyIndices,
                                const vk::SampleCountFlagBits maxMsaaSamples) noexcept
            : physicalDevice{ physicalDevice }, queueFamilyIndices{ queueFamilyIndices },
              maxMsaaSamples{ maxMsaaSamples } {
        }

        vk::PhysicalDevice physicalDevice;
        th::vulkan::QueueFamilyIndices queueFamilyIndices;
        vk::SampleCountFlagBits maxMsaaSamples;

        [[nodiscard]] auto createLogicalDevice() const -> vk::UniqueDevice;
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
        th::vulkan::QueueFamilyIndices queueFamilyIndices;
        vk::SampleCountFlagBits maxMsaaSamples;
        vk::UniqueCommandPool commandPool;

        [[nodiscard]] auto getGraphicQueue() const noexcept -> vk::Queue {
            return logicalDevice->getQueue(queueFamilyIndices.graphicFamily.value(), 0);
        }

        [[nodiscard]] auto getPresentationQueue() const noexcept -> vk::Queue {
            return logicalDevice->getQueue(queueFamilyIndices.presentFamily.value(), 0);
        }
    };

public:
    explicit PhysicalDevicesManager(const vk::Instance instance, const std::optional<vk::SurfaceKHR> surface)
        : m_physicalDevices{ enumeratePhysicalDevices(instance, surface) },
          m_selectedDevice{ VulkanInternalDevice{ m_physicalDevices.front() } } {}

    [[nodiscard]] auto getSelectedDevice() const noexcept -> Device {
        return Device{
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
    [[nodiscard]] auto enumeratePhysicalDevices(vk::Instance instance, std::optional<vk::SurfaceKHR> surface) const
            -> std::vector<PhysicalDevice>;

private:
    std::vector<PhysicalDevice> m_physicalDevices{};
    VulkanInternalDevice m_selectedDevice;
};

static constexpr auto g_sDeviceExtensions = std::array{ vk::KHRSwapchainExtensionName,
                                                        vk::KHRDynamicRenderingExtensionName,
                                                        vk::KHRSynchronization2ExtensionName,
                                                        vk::KHRBufferDeviceAddressExtensionName };

[[nodiscard]] auto PhysicalDevicesManager::PhysicalDevice::createLogicalDevice() const -> vk::UniqueDevice {
    const std::set indices = { queueFamilyIndices.graphicFamily.value(), queueFamilyIndices.presentFamily.value() };
    std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos;
    for (const auto ind : indices) {
        float queuePriority{ 1.0 };
        deviceQueueCreateInfos.emplace_back(vk::DeviceQueueCreateInfo{
                .queueFamilyIndex = ind,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority,
        });
    }

    const auto features = physicalDevice.getFeatures();
    constexpr auto vulkan13Features =
            vk::PhysicalDeviceVulkan13Features{ .synchronization2 = true, .dynamicRendering = true };
    constexpr auto vulkan12Features =
            vk::PhysicalDeviceVulkan12Features{ .descriptorIndexing = true, .bufferDeviceAddress = true };
    ;
    const auto deviceCreateInfo = vk::StructureChain(
            vk::DeviceCreateInfo{ .queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size()),
                                  .pQueueCreateInfos = deviceQueueCreateInfos.data(),
                                  .enabledExtensionCount = static_cast<uint32_t>(g_sDeviceExtensions.size()),
                                  .ppEnabledExtensionNames = g_sDeviceExtensions.data(),
                                  .pEnabledFeatures = &features },
            vulkan13Features,
            vulkan12Features);

    return physicalDevice.createDeviceUnique(deviceCreateInfo.get<vk::DeviceCreateInfo>());
}

static auto deviceHasAllRequiredExtensions(const vk::PhysicalDevice& physicalDevice) -> bool {
    const auto& availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    return std::ranges::all_of(g_sDeviceExtensions, [&availableDeviceExtensions](const auto& extension) {
        return std::ranges::any_of(availableDeviceExtensions, [&extension](const auto& instanceExtension) {
            return std::string_view(extension) == std::string_view(instanceExtension.extensionName);
        });
    });
}

static auto getMaxUsableSampleCount(const vk::PhysicalDevice& device) -> vk::SampleCountFlagBits {
    const auto counts = device.getProperties().limits.framebufferColorSampleCounts
                        & device.getProperties().limits.framebufferDepthSampleCounts;
    if (counts & vk::SampleCountFlagBits::e64) {
        return vk::SampleCountFlagBits::e64;
    }
    if (counts & vk::SampleCountFlagBits::e32) {
        return vk::SampleCountFlagBits::e32;
    }
    if (counts & vk::SampleCountFlagBits::e16) {
        return vk::SampleCountFlagBits::e16;
    }
    if (counts & vk::SampleCountFlagBits::e8) {
        return vk::SampleCountFlagBits::e8;
    }
    if (counts & vk::SampleCountFlagBits::e4) {
        return vk::SampleCountFlagBits::e4;
    }
    if (counts & vk::SampleCountFlagBits::e2) {
        return vk::SampleCountFlagBits::e2;
    }
    return vk::SampleCountFlagBits::e1;
}

auto PhysicalDevicesManager::enumeratePhysicalDevices(const vk::Instance instance,
                                                      const std::optional<vk::SurfaceKHR>
                                                              surface) const -> std::vector<PhysicalDevice> {
    static std::map<vk::PhysicalDeviceType, uint32_t> priorities = {
        { vk::PhysicalDeviceType::eOther, 0 },       { vk::PhysicalDeviceType::eCpu, 1 },
        { vk::PhysicalDeviceType::eVirtualGpu, 2 },  { vk::PhysicalDeviceType::eIntegratedGpu, 3 },
        { vk::PhysicalDeviceType::eDiscreteGpu, 4 },
    };

    std::vector<PhysicalDevice> physicalDevices;
    for (const auto& device : instance.enumeratePhysicalDevices()) {
        const auto queueFamilyIndex = th::vulkan::QueueFamilyIndices(device, surface);
        const auto deviceSupportExtensions = deviceHasAllRequiredExtensions(device);
        const auto swapChainSupportDetailsValid =
                surface.has_value() ? th::vulkan::SwapChainSupportDetails(device, surface.value()).isValid() : true;
        const auto maxMsaaSamples = getMaxUsableSampleCount(device);
        if (queueFamilyIndex.isCompleted() && deviceSupportExtensions && swapChainSupportDetailsValid
            && device.getFeatures().samplerAnisotropy) {
            physicalDevices.emplace_back(device, queueFamilyIndex, maxMsaaSamples);
        }
    }

    if (physicalDevices.empty()) {
        constexpr auto message = "No physical device exist which can meet all requirements!";
        TH_API_LOG_ERROR(message);
        throw std::runtime_error(message);
    }

    std::ranges::sort(physicalDevices, [](const auto& device1, const auto& device2) {
        const auto dt1 = device1.physicalDevice.getProperties().deviceType;
        const auto dt2 = device2.physicalDevice.getProperties().deviceType;
        return priorities[dt1] > priorities[dt2];
    });

    return physicalDevices;
}

}// namespace th::render_system::vulkan
