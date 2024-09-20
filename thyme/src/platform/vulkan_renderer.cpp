#include "thyme/platform/vulkan_renderer.hpp"
#include <set>

using namespace Thyme::Vulkan;

static auto getDevicesExtenstions() {
    static std::vector<const char*> deviceExtension = { vk::KHRSwapchainExtensionName };
    return deviceExtension;
}

UniqueInstance::UniqueInstance(const UniqueInstanceConfig& config) {
    constexpr auto appVersion = vk::makeApiVersion(0, Version::major, Version::minor, Version::patch);
    constexpr auto vulkanVersion = vk::makeApiVersion(0, 1, 3, 290);
    const vk::ApplicationInfo applicationInfo(
            config.appName.data(), appVersion, config.engineName.data(), appVersion, vulkanVersion);

    const vk::InstanceCreateInfo instanceCreateInfo(
            vk::InstanceCreateFlags(), &applicationInfo, config.instanceLayers, config.instanceExtension);
    try {
        instance = vk::createInstanceUnique(instanceCreateInfo);
    } catch (vk::SystemError err) {
        TH_API_LOG_ERROR("Failed to create vulkan instance. Message: {}, Code: {}", err.what(), err.code().value());
        throw std::runtime_error("Failed to create vulkan instance.");
    }
}

QueueFamilyIndices::QueueFamilyIndices(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface) {
    auto queueFamilies = device.getQueueFamilyProperties2();
    uint32_t i{ 0 };

    for (const auto& queueFamily : queueFamilies) {
        const auto& queueFamilyProperties = queueFamily.queueFamilyProperties;
        if (queueFamilyProperties.queueCount <= 0) {
            ++i;
            continue;
        }
        if (queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicFammily = i;
        }
        if (device.getSurfaceSupportKHR(i, surface)) {
            presentFamily = i;
        }
        if (isCompleted()) {
            break;
        }
    }
}

std::vector<PhysicalDevice> Thyme::Vulkan::getPhysicalDevices(const vk::UniqueInstance& instance,
                                                              const vk::UniqueSurfaceKHR& surface) {
    static std::map<vk::PhysicalDeviceType, uint32_t> priorities = {
        { vk::PhysicalDeviceType::eOther, 0 },         { vk::PhysicalDeviceType::eCpu, 1 },
        { vk::PhysicalDeviceType::eVirtualGpu, 2 },    { vk::PhysicalDeviceType::eDiscreteGpu, 3 },
        { vk::PhysicalDeviceType::eIntegratedGpu, 4 },
    };

    std::vector<PhysicalDevice> physicalDevices;

    for (const auto& device : instance.get().enumeratePhysicalDevices()) {
        const auto queueFamilyIndex = QueueFamilyIndices(device, *surface);
        if (queueFamilyIndex.isCompleted()) {
            physicalDevices.emplace_back(device, queueFamilyIndex);
        }
    }

    std::sort(physicalDevices.begin(), physicalDevices.end(), [](const auto& device1, const auto& device2) {
        const auto dt1 = device1.physicalDevice.getProperties().deviceType;
        const auto dt2 = device2.physicalDevice.getProperties().deviceType;
        return priorities[dt1] < priorities[dt2];
    });

    return physicalDevices;
}

vk::Device Thyme::Vulkan::PhysicalDevice::createLogicalDevice() {
    std::set<uint32_t> indices = { queueFamilyIndices.graphicFammily.value(),
                                   queueFamilyIndices.presentFamily.value() };
    std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos;
    for (const auto ind : indices) {
        float queuePriority{ 1.0 };
        deviceQueueCreateInfos.emplace_back(vk::DeviceQueueCreateFlags(), ind, 1, &queuePriority);
    }

    const auto features = physicalDevice.getFeatures();
    const auto deviceExtensions = getDevicesExtenstions();

    return physicalDevice.createDevice(vk::DeviceCreateInfo(vk::DeviceCreateFlags(),
                                                                  static_cast<uint32_t>(deviceQueueCreateInfos.size()),
                                                                  deviceQueueCreateInfos.data(),
                                                                  0,
                                                                  nullptr,
                                                                  static_cast<uint32_t>(deviceExtensions.size()),
                                                                  deviceExtensions.data(),
                                                                  &features));
}
