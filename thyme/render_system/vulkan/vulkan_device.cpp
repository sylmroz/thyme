module;

module th.render_system.vulkan;

import th.core.logger;

namespace th {

static constexpr auto g_sDeviceExtensions = std::array{ vk::KHRSwapchainExtensionName,
                                                        vk::KHRDynamicRenderingExtensionName,
                                                        vk::KHRSynchronization2ExtensionName,
                                                        vk::KHRBufferDeviceAddressExtensionName };

static auto deviceHasAllRequiredExtensions(const vk::PhysicalDevice physical_device) -> bool {
    const auto& available_device_extensions = physical_device.enumerateDeviceExtensionProperties();
    return std::ranges::all_of(g_sDeviceExtensions, [&available_device_extensions](const auto& extension) {
        return std::ranges::any_of(available_device_extensions, [&extension](const auto& instanceExtension) {
            return std::string_view(extension) == std::string_view(instanceExtension.extensionName);
        });
    });
}

auto getMaxUsableSampleCount(const vk::PhysicalDevice device) noexcept -> vk::SampleCountFlagBits {
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

[[nodiscard]] auto hasRequiredFeatures(const vk::PhysicalDevice physical_device) noexcept -> bool {
    const auto& features = physical_device.getFeatures();
    return features.samplerAnisotropy;
}

static auto hasRequiredQueues(const vk::PhysicalDevice physical_device,
                              const std::span<const vk::QueueFlags>
                                      queue_flag_bits) noexcept -> bool {
    const auto queue_family_properties = physical_device.getQueueFamilyProperties();
    return std::ranges::all_of(queue_flag_bits, [&queue_family_properties](const vk::QueueFlags queue_flag_bit) {
        return std::ranges::any_of(queue_family_properties,
                                   [queue_flag_bit](const vk::QueueFamilyProperties& queue_family_property) {
                                       return !!(queue_family_property.queueFlags & queue_flag_bit);
                                   });
    });
}

auto filterDevices(const std::span<const vk::raii::PhysicalDevice> physical_devices, const vk::SurfaceKHR surface)
        -> std::vector<vk::raii::PhysicalDevice> {
    const auto filtered_devices =
            physical_devices | std::ranges::views::filter([surface](auto& physical_device) -> bool {
                const auto deviceSupportExtensions = deviceHasAllRequiredExtensions(physical_device);
                const auto has_all_required_features = hasRequiredFeatures(physical_device);
                const auto is_suitable_for_surface = isPhysicalDeviceSuitable(physical_device, surface);
                constexpr auto required_queues =
                        std::array{ vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute };
                const auto has_required_queues = hasRequiredQueues(physical_device, required_queues);
                return deviceSupportExtensions && has_all_required_features && is_suitable_for_surface
                       && has_required_queues;
            })
            | std::ranges::to<std::vector<vk::raii::PhysicalDevice>>();
    return filtered_devices;
}

auto createLogicalDevice(const vk::raii::PhysicalDevice& physical_device, uint32_t queue_index) -> vk::raii::Device {
    float queue_priority{ 1.0 };
    const auto device_queue_create_infos = std::array{ vk::DeviceQueueCreateInfo{
            .queueFamilyIndex = queue_index,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority,
    } };


    const auto physical_device_features = physical_device.getFeatures();
    const auto features = vk::PhysicalDeviceFeatures2{
        .features = vk::PhysicalDeviceFeatures{
            .sampleRateShading = physical_device_features.sampleRateShading,
            .fillModeNonSolid = physical_device_features.fillModeNonSolid,
            .wideLines = physical_device_features.wideLines,
            .largePoints = physical_device_features.largePoints,
            .samplerAnisotropy = physical_device_features.samplerAnisotropy,
        }
    };

    constexpr auto vulkan13_features =
            vk::PhysicalDeviceVulkan13Features{ .synchronization2 = true, .dynamicRendering = true };
    constexpr auto vulkan12_features =
            vk::PhysicalDeviceVulkan12Features{ .descriptorIndexing = true, .bufferDeviceAddress = true };

    const auto feature_chain = vk::StructureChain{ features, vulkan13_features, vulkan12_features };

    const auto device_create_info = vk::StructureChain(
            vk::DeviceCreateInfo{ .queueCreateInfoCount = static_cast<uint32_t>(device_queue_create_infos.size()),
                                  .pQueueCreateInfos = device_queue_create_infos.data(),
                                  .enabledExtensionCount = static_cast<uint32_t>(g_sDeviceExtensions.size()),
                                  .ppEnabledExtensionNames = g_sDeviceExtensions.data() },
            feature_chain.get<vk::PhysicalDeviceFeatures2>());
    return vk::raii::Device(physical_device, device_create_info.get<vk::DeviceCreateInfo>());
}

PhysicalDevice2::PhysicalDevice2(const vk::raii::PhysicalDevice& physical_device)
    : physical_device(physical_device), max_msaa_samples(getMaxUsableSampleCount(physical_device)),
      device_name(physical_device.getProperties().deviceName.data()) {}

PhysicalDevices2::PhysicalDevices2(std::span<const vk::raii::PhysicalDevice> physical_devices)
    : physical_devices(sortPhysicalDevices(physical_devices)) {}

std::vector<PhysicalDevice2>
        PhysicalDevices2::sortPhysicalDevices(const std::span<const vk::raii::PhysicalDevice> physical_devices) {
    auto filtered_devices = physical_devices | std::views::transform([](const auto& physical_device) {
                                return PhysicalDevice2(physical_device);
                            })
                            | std::ranges::to<std::vector<PhysicalDevice2>>();
    static std::unordered_map<vk::PhysicalDeviceType, uint32_t> priorities = {
        { vk::PhysicalDeviceType::eOther, 0 },       { vk::PhysicalDeviceType::eCpu, 1 },
        { vk::PhysicalDeviceType::eVirtualGpu, 2 },  { vk::PhysicalDeviceType::eIntegratedGpu, 3 },
        { vk::PhysicalDeviceType::eDiscreteGpu, 4 },
    };

    std::ranges::sort(filtered_devices, [](const auto& device1, const auto& device2) {
        const auto dt1 = device1.physical_device.getProperties().deviceType;
        const auto dt2 = device2.physical_device.getProperties().deviceType;
        return priorities[dt1] > priorities[dt2];
    });
    return filtered_devices;
}

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

    const auto features = physical_device.getFeatures2();
    constexpr auto vulkan13Features =
            vk::PhysicalDeviceVulkan13Features{ .synchronization2 = true, .dynamicRendering = true };
    constexpr auto vulkan12Features =
            vk::PhysicalDeviceVulkan12Features{ .descriptorIndexing = true, .bufferDeviceAddress = true };

    const auto featureChain = vk::StructureChain{ features, vulkan13Features, vulkan12Features };

    const auto deviceCreateInfo = vk::StructureChain(
            vk::DeviceCreateInfo{ .queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size()),
                                  .pQueueCreateInfos = deviceQueueCreateInfos.data(),
                                  .enabledExtensionCount = static_cast<uint32_t>(g_sDeviceExtensions.size()),
                                  .ppEnabledExtensionNames = g_sDeviceExtensions.data() },
            featureChain.get<vk::PhysicalDeviceFeatures2>());
    return vk::raii::Device(physical_device, deviceCreateInfo.get<vk::DeviceCreateInfo>());
}

auto VulkanPhysicalDevicesManager::enumeratePhysicalDevices(const std::span<const vk::raii::PhysicalDevice>
                                                                    physical_devices,
                                                            const vk::SurfaceKHR surface) const
        -> std::vector<PhysicalDevice> {

    const auto filtered_devices = filterDevices(physical_devices, surface);

    static std::unordered_map<vk::PhysicalDeviceType, uint32_t> priorities = {
        { vk::PhysicalDeviceType::eOther, 0 },       { vk::PhysicalDeviceType::eCpu, 1 },
        { vk::PhysicalDeviceType::eVirtualGpu, 2 },  { vk::PhysicalDeviceType::eIntegratedGpu, 3 },
        { vk::PhysicalDeviceType::eDiscreteGpu, 4 },
    };

    std::vector<PhysicalDevice> physicalDevices;
    for (const auto& device : filtered_devices) {
        const auto queueFamilyIndex = QueueFamilyIndices(device, surface);
        const auto maxMsaaSamples = getMaxUsableSampleCount(device);
        physicalDevices.emplace_back(device, queueFamilyIndex, maxMsaaSamples);
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
