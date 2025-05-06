#include <thyme/platform/vulkan/vulkan_device.hpp>

namespace th::vulkan {

static constexpr auto s_deviceExtensions =
        std::array{ vk::KHRSwapchainExtensionName, vk::KHRDynamicRenderingExtensionName };

[[nodiscard]] vk::UniqueDevice PhysicalDevicesManager::PhysicalDevice::createLogicalDevice() const {
    const std::set indices = { queueFamilyIndices.graphicFamily.value(), queueFamilyIndices.presentFamily.value() };
    std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos;
    for (const auto ind : indices) {
        float queuePriority{ 1.0 };
        deviceQueueCreateInfos.emplace_back(vk::DeviceQueueCreateFlags(), ind, 1, &queuePriority);
    }

    const auto features = physicalDevice.getFeatures();
    constexpr auto dynamicRenderingFeatures = vk::PhysicalDeviceDynamicRenderingFeaturesKHR(true);
    const auto deviceCreateInfo = vk::StructureChain(
            vk::DeviceCreateInfo(
                    vk::DeviceCreateFlags(), deviceQueueCreateInfos, nullptr, s_deviceExtensions, &features),
            dynamicRenderingFeatures);

    return physicalDevice.createDeviceUnique(deviceCreateInfo.get<vk::DeviceCreateInfo>());
}

PhysicalDevicesManager::PhysicalDevicesManager(const vk::Instance instance, const vk::SurfaceKHR surface)
    : m_physicalDevices{ getPhysicalDevices(instance, surface) },
      m_selectedDevice{ VulkanInternalDevice{ m_physicalDevices.front() } } {}

static bool deviceHasAllRequiredExtensions(const vk::PhysicalDevice& physicalDevice) {
    const auto& availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    return std::ranges::all_of(s_deviceExtensions, [&availableDeviceExtensions](const auto& extension) {
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


auto PhysicalDevicesManager::getPhysicalDevices(const vk::Instance instance, const vk::SurfaceKHR surface) const
        -> std::vector<PhysicalDevice> {
    static std::map<vk::PhysicalDeviceType, uint32_t> priorities = {
        { vk::PhysicalDeviceType::eOther, 0 },       { vk::PhysicalDeviceType::eCpu, 1 },
        { vk::PhysicalDeviceType::eVirtualGpu, 2 },  { vk::PhysicalDeviceType::eIntegratedGpu, 3 },
        { vk::PhysicalDeviceType::eDiscreteGpu, 4 },
    };

    std::vector<PhysicalDevice> physicalDevices;
    for (const auto& device : instance.enumeratePhysicalDevices()) {
        const auto queueFamilyIndex = QueueFamilyIndices(device, surface);
        const auto deviceSupportExtensions = deviceHasAllRequiredExtensions(device);
        const auto swapChainSupportDetails = SwapChainSupportDetails(device, surface);
        const auto maxMsaaSamples = getMaxUsableSampleCount(device);
        if (queueFamilyIndex.isCompleted() && deviceSupportExtensions && swapChainSupportDetails.isValid()
            && device.getFeatures().samplerAnisotropy) {
            physicalDevices.emplace_back(device, queueFamilyIndex, maxMsaaSamples);
        }
    }

    if (physicalDevices.empty()) {
        constexpr auto message = "No physical device exist which mets all requirements!";
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
}// namespace th::vulkan