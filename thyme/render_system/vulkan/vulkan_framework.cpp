module;

module th.render_system.vulkan;

#if !defined(NDEBUG)
constexpr auto g_defaultEnabledExtensions =
        std::array{ vk::KHRPortabilityEnumerationExtensionName, vk::EXTDebugUtilsExtensionName };
constexpr auto g_validationLayers = std::array{ "VK_LAYER_KHRONOS_validation" };
#else
constexpr auto g_defaultEnabledExtensions = std::array{ vk::KHRPortabilityEnumerationExtensionName };
#endif

namespace th {
VulkanFramework::VulkanFramework(const InitInfo& info, Logger& logger) : VulkanFramework(info, {}, logger) {}

VulkanFramework::VulkanFramework(const InitInfo& info,
                                 const std::vector<std::string>& window_extensions,
                                 Logger& logger)
    : m_logger{ logger }, m_instance{ createInstance(info, window_extensions) }
#if !defined(NDEBUG)
      ,
      m_debug{ m_instance, logger }
#endif
{
    logger.info("Vulkan framework initialized:\n\tAppName: {}\n\t{}\n\t{} {} {} {}",
                info.app_name,
                info.engine_name,
                0,// version::major,
                1,// version::minor,
                0,// version::patch,
                0);// version::tweak);
}

[[nodiscard]] auto VulkanFramework::createInstance(const InitInfo& info,
                                                   const std::vector<std::string>& window_extensions) const
        -> vk::raii::Instance {
    dumpExtensions();
    dumpLayers();

    const auto enabled_extensions = mergeInstanceExtensions(g_defaultEnabledExtensions, window_extensions);
    if (!validateExtension(enabled_extensions)) {
        throw std::runtime_error("All required extensions are not supported by device");
    }

    // constexpr auto appVersion = vk::makeApiVersion(version::major, version::minor, version::patch, version::tweak);
    constexpr auto app_version = vk::makeApiVersion(0, 1, 0, 0);
    const auto application_info = vk::ApplicationInfo{
        .pApplicationName = info.app_name.c_str(),
        .applicationVersion = app_version,
        .pEngineName = info.engine_name.c_str(),
        .engineVersion = app_version,
        .apiVersion = vk::HeaderVersionComplete,
    };

    const auto instance_create_info = [&application_info, &enabled_extensions, this] {
#if !defined(NDEBUG)
        const auto enable_validation = validateLayers(g_validationLayers);
        if (!enable_validation) {
            m_logger.warn("Validation layers are not available! Proceeding without them.");
        }
        if (enable_validation) {
            return vk::InstanceCreateInfo{
                .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
                .pApplicationInfo = &application_info,
                .enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size()),
                .ppEnabledLayerNames = g_validationLayers.data(),
                .enabledExtensionCount = static_cast<uint32_t>(enabled_extensions.size()),
                .ppEnabledExtensionNames = enabled_extensions.data(),
            };
        }
#endif
        return vk::InstanceCreateInfo{
            .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
            .pApplicationInfo = &application_info,
            .enabledExtensionCount = static_cast<uint32_t>(enabled_extensions.size()),
            .ppEnabledExtensionNames = enabled_extensions.data(),
        };
    }();
#if !defined(NDEBUG)
    constexpr auto enabled_validation_features = std::array{ vk::ValidationFeatureEnableEXT::eSynchronizationValidation };
    const auto instance_create_info_chain = vk::StructureChain(
            instance_create_info,
            VulkanDebug::createDebugUtilsMessengerCreateInfo(&m_logger),
            vk::ValidationFeaturesEXT{
                    .enabledValidationFeatureCount = static_cast<uint32_t>(enabled_validation_features.size()),
                    .pEnabledValidationFeatures = enabled_validation_features.data(),
            });
#else
    const vk::StructureChain instanceCreateInfoChain(instance_create_info);
#endif
    return m_context.createInstance(instance_create_info_chain.get<vk::InstanceCreateInfo>());
}

void VulkanFramework::dumpExtensions() const {
    m_logger.info("Vulkan extensions:");
    for (const auto& extension : m_context.enumerateInstanceExtensionProperties()) {
        m_logger.info("\t{}", std::string_view(extension.extensionName));
    }
}

void VulkanFramework::dumpLayers() const {
    m_logger.info("Vulkan layers:");
    for (const auto& layer : m_context.enumerateInstanceLayerProperties()) {
        m_logger.info("\t{}", std::string_view(layer.layerName));
    }
}

auto VulkanFramework::mergeInstanceExtensions(const std::span<const char* const> default_extensions,
                                              const std::span<const std::string>
                                                      window_extensions) -> std::vector<const char*> {
    {
        std::vector merged_extensions(default_extensions.begin(), default_extensions.end());
        if (window_extensions.empty()) {
            return merged_extensions;
        }
        for (const auto& extension : window_extensions) {
            merged_extensions.push_back(extension.c_str());
        }
        return merged_extensions;
    }
}

auto VulkanFramework::validateExtension(std::span<const char* const> requested_extensions) const -> bool {

    const auto extension_properties = m_context.enumerateInstanceExtensionProperties()
                                      | std::views::transform([](const vk::ExtensionProperties& extension) {
                                            return std::string_view(extension.extensionName);
                                        });
    const auto requested_extensions_range = requested_extensions | std::views::transform([](const auto& extension) {
                                                return std::string_view(extension);
                                            });
    return arrayContainsArray(requested_extensions_range, extension_properties);
}

auto VulkanFramework::validateLayers(std::span<const char* const> requested_layers) const -> bool {

    const auto layer_properties_names =
            m_context.enumerateInstanceLayerProperties()
            | std::views::transform([](const vk::LayerProperties& layer) { return std::string_view(layer.layerName); });
    const auto requested_layers_range =
            requested_layers | std::views::transform([](const auto& extension) { return std::string_view(extension); });
    return arrayContainsArray(requested_layers_range, layer_properties_names);
}

}// namespace th
