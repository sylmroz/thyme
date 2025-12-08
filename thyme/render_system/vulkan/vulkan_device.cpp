module;

module th.render_system.vulkan;

import th.core.logger;

namespace th {

static constexpr auto g_sDeviceExtensions = std::array{ vk::KHRSwapchainExtensionName,
                                                        vk::KHRDynamicRenderingExtensionName,
                                                        vk::KHRSynchronization2ExtensionName,
                                                        vk::KHRBufferDeviceAddressExtensionName };

[[nodiscard]] auto PhysicalDevice::createLogicalDevice() const -> vk::raii::Device {
    const std::set<uint32_t> indices = { queue_family_indices.graphic_family.value(),
                                         queue_family_indices.present_family.value() };
    std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos;
    for (const auto ind : indices) {
        float queuePriority{ 1.0 };
        deviceQueueCreateInfos.emplace_back(vk::DeviceQueueCreateInfo{
                .queueFamilyIndex = ind,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority,
        });
    }

    const auto features = physical_device.getFeatures();
    constexpr auto vulkan13Features =
            vk::PhysicalDeviceVulkan13Features{ .synchronization2 = true, .dynamicRendering = true };
    constexpr auto vulkan12Features =
            vk::PhysicalDeviceVulkan12Features{ .descriptorIndexing = true, .bufferDeviceAddress = true };
    const auto deviceCreateInfo = vk::StructureChain(
            vk::DeviceCreateInfo{ .queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size()),
                                  .pQueueCreateInfos = deviceQueueCreateInfos.data(),
                                  .enabledExtensionCount = static_cast<uint32_t>(g_sDeviceExtensions.size()),
                                  .ppEnabledExtensionNames = g_sDeviceExtensions.data(),
                                  .pEnabledFeatures = &features },
            vulkan13Features,
            vulkan12Features);
    return vk::raii::Device(physical_device, deviceCreateInfo.get<vk::DeviceCreateInfo>());
}

static auto deviceHasAllRequiredExtensions(const vk::PhysicalDevice& physical_device) -> bool {
    const auto& available_device_extensions = physical_device.enumerateDeviceExtensionProperties();
    return std::ranges::all_of(g_sDeviceExtensions, [&available_device_extensions](const auto& extension) {
        return std::ranges::any_of(available_device_extensions, [&extension](const auto& instanceExtension) {
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

auto VulkanPhysicalDevicesManager::enumeratePhysicalDevices(const vk::raii::Instance& instance,
                                                            const std::optional<vk::SurfaceKHR>
                                                                    surface) const -> std::vector<PhysicalDevice> {
    static std::map<vk::PhysicalDeviceType, uint32_t> priorities = {
        { vk::PhysicalDeviceType::eOther, 0 },       { vk::PhysicalDeviceType::eCpu, 1 },
        { vk::PhysicalDeviceType::eVirtualGpu, 2 },  { vk::PhysicalDeviceType::eIntegratedGpu, 3 },
        { vk::PhysicalDeviceType::eDiscreteGpu, 4 },
    };

    std::vector<PhysicalDevice> physicalDevices;
    for (const auto& device : instance.enumeratePhysicalDevices()) {
        const auto queueFamilyIndex = QueueFamilyIndices(device, surface);
        const auto deviceSupportExtensions = deviceHasAllRequiredExtensions(device);
        const auto swapChainSupportDetailsValid =
                surface.has_value() ? SwapChainSupportDetails(device, surface.value()).isValid() : true;
        const auto maxMsaaSamples = getMaxUsableSampleCount(device);
        if (queueFamilyIndex.isCompleted() && deviceSupportExtensions && swapChainSupportDetailsValid
            && device.getFeatures().samplerAnisotropy) {
            physicalDevices.emplace_back(device, queueFamilyIndex, maxMsaaSamples);
        }
    }

    if (physicalDevices.empty()) {
        constexpr auto message = "No physical device exist which can meet all requirements!";
        m_logger.error(message);
        throw std::runtime_error(message);
    }

    std::ranges::sort(physicalDevices, [](const auto& device1, const auto& device2) {
        const auto dt1 = device1.physical_device.getProperties().deviceType;
        const auto dt2 = device2.physical_device.getProperties().deviceType;
        return priorities[dt1] > priorities[dt2];
    });

    return physicalDevices;
}

}// namespace th
