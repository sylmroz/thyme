module;

#include "thyme/core/logger.hpp"
#include "thyme/version.hpp"

#include <map>
#include <set>
#include <vulkan/vulkan.hpp>

module thyme.platform.vulkan_renderer;

using namespace Thyme::Vulkan;

#if !defined(NDEBUG)
PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                                              const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                              const VkAllocationCallbacks* pAllocator,
                                                              VkDebugUtilsMessengerEXT* pMessenger) {
    return pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
                                                           VkDebugUtilsMessengerEXT messenger,
                                                           VkAllocationCallbacks const* pAllocator) {
    return pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(const vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    const vk::DebugUtilsMessageTypeFlagBitsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData) {
    static std::map<vk::DebugUtilsMessageTypeFlagBitsEXT, std::string_view> messageTypeStringMap = {
        { vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral, "General" },
        { vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance, "Performance" },
        { vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation, "Validation" },
        { vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding, "Device Address Biding" },
    };
    const auto messageTypeStr = messageTypeStringMap[messageType];
    switch (messageSeverity) {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        TH_API_LOG_TRACE(
                "[{}]: Name: {}, Message: {}", messageTypeStr, pCallbackData->pMessageIdName, pCallbackData->pMessage);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        TH_API_LOG_INFO(
                "[{}]: Name: {}, Message: {}", messageTypeStr, pCallbackData->pMessageIdName, pCallbackData->pMessage);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        TH_API_LOG_WARN(
                "[{}]: Name: {}, Message: {}", messageTypeStr, pCallbackData->pMessageIdName, pCallbackData->pMessage);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        TH_API_LOG_ERROR(
                "[{}]: Name: {}, Message: {}", messageTypeStr, pCallbackData->pMessageIdName, pCallbackData->pMessage);
        break;
    default: break;
    }
    return VK_FALSE;
}
#endif

static auto getDeviceExtensions() {
    static const std::vector<const char*> deviceExtension = { vk::KHRSwapchainExtensionName };
    return deviceExtension;
}

UniqueInstance::UniqueInstance(const UniqueInstanceConfig& config) {
    constexpr auto appVersion = vk::makeApiVersion(0, Version::major, Version::minor, Version::patch);
    const vk::ApplicationInfo applicationInfo(
            config.appName.data(), appVersion, config.engineName.data(), appVersion, vk::HeaderVersionComplete);

    auto enabledExtensions = config.instanceExtension;
#if !defined(NDEBUG)
    enabledExtensions.emplace_back(vk::EXTDebugUtilsExtensionName);
#endif

    const vk::InstanceCreateInfo instanceCreateInfo(
            vk::InstanceCreateFlags(), &applicationInfo, config.instanceLayers, enabledExtensions);
    try {
        instance = vk::createInstanceUnique(instanceCreateInfo);
#if !defined(NDEBUG)
        setupDebugMessenger();
#endif
    } catch (const vk::SystemError& err) {
        const auto message =
                fmt::format("Failed to create vulkan instance. Message: {}, Code: {}", err.what(), err.code().value());
        TH_API_LOG_ERROR(message);
        throw std::runtime_error(message);
    }
}

#if !defined(NDEBUG)
void UniqueInstance::validateExtensions() {
    const auto extensionProperties = vk::enumerateInstanceExtensionProperties();
    const auto it = std::ranges::find_if(extensionProperties, [](const vk::ExtensionProperties& extension) {
        return std::string_view(extension.extensionName) == std::string_view(vk::EXTDebugUtilsExtensionName);
    });
    if (it == extensionProperties.end()) {
        const auto message = fmt::format("No extension {} found", vk::EXTDebugUtilsExtensionName);
        TH_API_LOG_ERROR(message);
        throw std::runtime_error(message);
    }
}
void UniqueInstance::setupDebugMessenger() {
    validateExtensions();

    pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            instance->getProcAddr("vkCreateDebugUtilsMessengerEXT"));
    if (!pfnVkCreateDebugUtilsMessengerEXT) {
        constexpr auto message = "Failed to get vkCreateDebugUtilsMessengerEXT function.";
        TH_API_LOG_ERROR(message);
        throw std::runtime_error(message);
    }

    pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            instance->getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
    if (!pfnVkDestroyDebugUtilsMessengerEXT) {
        constexpr auto message = "Failed to get vkDestroyDebugUtilsMessengerEXT function.";
        TH_API_LOG_ERROR(message);
        throw std::runtime_error(message);
    }

    const auto flags =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
    const auto typeFlags = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                           | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
                           | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    const vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo(
            vk::DebugUtilsMessengerCreateFlagsEXT(),
            flags,
            typeFlags,
            reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(debugCallback));

    debugMessenger = instance->createDebugUtilsMessengerEXTUnique(debugMessengerCreateInfo);
}
#endif

QueueFamilyIndices::QueueFamilyIndices(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface) {
    const auto queueFamilies = device.getQueueFamilyProperties2();
    uint32_t i{ 0 };

    for (const auto& queueFamily : queueFamilies) {
        const auto& queueFamilyProperties = queueFamily.queueFamilyProperties;
        if (queueFamilyProperties.queueCount <= 0) {
            ++i;
            continue;
        }
        if (queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicFamily = i;
        }
        if (device.getSurfaceSupportKHR(i, surface) != 0u) {
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
        if (const auto queueFamilyIndex = QueueFamilyIndices(device, *surface); queueFamilyIndex.isCompleted()) {
            physicalDevices.emplace_back(device, queueFamilyIndex);
        }
    }

    std::ranges::sort(physicalDevices, [](const auto& device1, const auto& device2) {
        const auto dt1 = device1.physicalDevice.getProperties().deviceType;
        const auto dt2 = device2.physicalDevice.getProperties().deviceType;
        return priorities[dt1] < priorities[dt2];
    });

    return physicalDevices;
}

[[nodiscard]] vk::UniqueDevice Thyme::Vulkan::PhysicalDevice::createLogicalDevice() const {
    std::set<uint32_t> const indices = { queueFamilyIndices.graphicFamily.value(),
                                         queueFamilyIndices.presentFamily.value() };
    std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos;
    for (const auto ind : indices) {
        float queuePriority{ 1.0 };
        deviceQueueCreateInfos.emplace_back(vk::DeviceQueueCreateFlags(), ind, 1, &queuePriority);
    }

    const auto features = physicalDevice.getFeatures();
    const auto deviceExtensions = getDeviceExtensions();

    return physicalDevice.createDeviceUnique(vk::DeviceCreateInfo(vk::DeviceCreateFlags(),
                                                                  static_cast<uint32_t>(deviceQueueCreateInfos.size()),
                                                                  deviceQueueCreateInfos.data(),
                                                                  0,
                                                                  nullptr,
                                                                  static_cast<uint32_t>(deviceExtensions.size()),
                                                                  deviceExtensions.data(),
                                                                  &features));
}
