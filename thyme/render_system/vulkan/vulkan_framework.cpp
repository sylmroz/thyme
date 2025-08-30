module;

#include <array>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

module th.render_system.vulkan;

#if !defined(NDEBUG)
constexpr auto defaultEnabledExtensions =
        std::array{ vk::KHRPortabilityEnumerationExtensionName, vk::EXTDebugUtilsExtensionName };
constexpr auto validationLayers = std::array{ "VK_LAYER_KHRONOS_validation" };
#else
constexpr auto defaultEnabledExtensions = std::array{ vk::KHRPortabilityEnumerationExtensionName };
#endif

namespace th {

VulkanFramework::VulkanFramework(const InitInfo& info) : VulkanFramework(info, {}) {}

VulkanFramework::VulkanFramework(const InitInfo& info, const std::vector<std::string>& windowExtensions)
    : m_instance{ createInstance(info, windowExtensions) }
#if !defined(NDEBUG)
      ,
      m_debug{ m_instance }
#endif
{
    ThymeLogger::getLogger()->info("Vulkan framework initialized:\n\tAppName: {}\n\t{}\n\t{} {} {} {}",
                                         info.appName,
                                         info.engineName,
                                         0,//version::major,
                                         1,//version::minor,
                                         0,//version::patch,
                                         0);//version::tweak);
}

[[nodiscard]] auto VulkanFramework::createInstance(const InitInfo& info,
                                                   const std::vector<std::string>& windowExtensions) const
        -> vk::raii::Instance {
    dumpExtensions();
    dumpLayers();

    const auto enabledExtensions = mergeInstanceExtensions(defaultEnabledExtensions, windowExtensions);
    if (!validateExtension(enabledExtensions)) {
        throw std::runtime_error("All required extensions are not supported by device");
    }

    //constexpr auto appVersion = vk::makeApiVersion(version::major, version::minor, version::patch, version::tweak);
    constexpr auto appVersion = vk::makeApiVersion(0, 1, 0, 0);
    const auto applicationInfo = vk::ApplicationInfo{
        .pApplicationName = info.appName.c_str(),
        .applicationVersion = appVersion,
        .pEngineName = info.engineName.c_str(),
        .engineVersion = appVersion,
        .apiVersion = vk::HeaderVersionComplete,
    };

    const auto instanceCreateInfo = [&applicationInfo, &enabledExtensions, this] {
#if !defined(NDEBUG)
        const auto enableValidation = validateLayers(validationLayers);
        if (!enableValidation) {
            ThymeLogger::getLogger()->warn("Validation layers are not available! Proceeding without them.");
        }
        if (enableValidation) {
            return vk::InstanceCreateInfo{
                .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
                .pApplicationInfo = &applicationInfo,
                .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
                .ppEnabledLayerNames = validationLayers.data(),
                .enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
                .ppEnabledExtensionNames = enabledExtensions.data(),
            };
        }
#endif
        return vk::InstanceCreateInfo{
            .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
            .pApplicationInfo = &applicationInfo,
            .enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
            .ppEnabledExtensionNames = enabledExtensions.data(),
        };
    }();
#if !defined(NDEBUG)
    constexpr auto enabledValidationFeatures = std::array{ vk::ValidationFeatureEnableEXT::eSynchronizationValidation };
    const auto instanceCreateInfoChain = vk::StructureChain(
            instanceCreateInfo,
            render_system::vulkan::Debug::createDebugUtilsMessengerCreateInfo(),
            vk::ValidationFeaturesEXT{
                    .enabledValidationFeatureCount = static_cast<uint32_t>(enabledValidationFeatures.size()),
                    .pEnabledValidationFeatures = enabledValidationFeatures.data(),
            });
#else
    const vk::StructureChain instanceCreateInfoChain(instanceCreateInfo);
#endif
    return m_context.createInstance(instanceCreateInfoChain.get<vk::InstanceCreateInfo>());
}

void VulkanFramework::dumpExtensions() const {
    ThymeLogger::getLogger()->info("Vulkan extensions:");
    for (const auto& extension : m_context.enumerateInstanceExtensionProperties()) {
        ThymeLogger::getLogger()->info("\t{}", std::string_view(extension.extensionName));
    }
}

void VulkanFramework::dumpLayers() const {
    ThymeLogger::getLogger()->info("Vulkan layers:");
    for (const auto& layer : m_context.enumerateInstanceLayerProperties()) {
        ThymeLogger::getLogger()->info("\t{}", std::string_view(layer.layerName));
    }
}

auto VulkanFramework::mergeInstanceExtensions(const std::span<const char* const> defaultExtensions,
                                              const std::span<const std::string>
                                                      windowExtensions) -> std::vector<const char*> {
    {
        std::vector mergedExtensions(defaultExtensions.begin(), defaultExtensions.end());
        if (windowExtensions.empty()) {
            return mergedExtensions;
        }
        for (const auto& extension : windowExtensions) {
            mergedExtensions.push_back(extension.c_str());
        }
        return mergedExtensions;
    }
}

auto VulkanFramework::validateExtension(std::span<const char* const> requestedExtensions) const -> bool {

    const auto extensionProperties = m_context.enumerateInstanceExtensionProperties()
                                     | std::views::transform([](const vk::ExtensionProperties& extension) {
                                           return std::string_view(extension.extensionName);
                                       });
    const auto requestedExtensionsRange = requestedExtensions | std::views::transform([](const auto& extension) {
                                              return std::string_view(extension);
                                          });
    return arrayContainsArray(requestedExtensionsRange, extensionProperties);
}

auto VulkanFramework::validateLayers(std::span<const char* const> requestedLayers) const -> bool {

    const auto layerPropertiesNames =
            m_context.enumerateInstanceLayerProperties()
            | std::views::transform([](const vk::LayerProperties& layer) { return std::string_view(layer.layerName); });
    const auto requestedLayersRange =
            requestedLayers | std::views::transform([](const auto& extension) { return std::string_view(extension); });
    return arrayContainsArray(requestedLayersRange, layerPropertiesNames);
}

}// namespace th
