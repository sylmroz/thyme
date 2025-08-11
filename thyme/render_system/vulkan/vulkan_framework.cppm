module;

#include "thyme/core/window.hpp"


#include <thyme/version.hpp>

#include <GLFW/glfw3.h>
#include <string>

#include <spdlog/spdlog.h>

export module th.render_system.vulkan.framework;

import th.core.logger;
import th.core.utils;

import th.render_system.framework;
#if !defined(NDEBUG)
import th.render_system.vulkan.debug;
#endif
import vulkan_hpp;

namespace th::render_system::vulkan {

template <typename IWindow>
concept WindowType = requires() {
    { IWindow::getExtensions() } -> std::same_as<std::vector<std::string>>;
};

template <typename IWindow>
concept CanPassWindow = WindowType<IWindow> || std::same_as<IWindow, void>;

[[nodiscard]] inline auto getSwapChainSupportExtensions() -> std::vector<std::string> {
    uint32_t instanceExtensionCount{ 0 };
    auto* instanceExtensionBuffer = glfwGetRequiredInstanceExtensions(&instanceExtensionCount);
    std::vector<std::string> instanceExtensions(instanceExtensionCount);
    std::copy_n(instanceExtensionBuffer, instanceExtensionCount, instanceExtensions.begin());
    return instanceExtensions;
}

#if !defined(NDEBUG)
constexpr auto defaultEnabledExtensions =
        std::array{ vk::KHRPortabilityEnumerationExtensionName, vk::EXTDebugUtilsExtensionName };
constexpr auto validationLayers = std::array{ "VK_LAYER_KHRONOS_validation" };
#else
constexpr auto defaultEnabledExtensions = std::array{ vk::KHRPortabilityEnumerationExtensionName };
#endif

export class Framework final: public render_system::Framework {
    using render_system::Framework::Framework;

public:
    template <CanPassWindow window = void>
    static auto create(const InitInfo& info) -> Framework {
        if constexpr (std::is_same_v<window, void>) {
            return Framework(info);
        }
        return Framework{ info, getSwapChainSupportExtensions() };
    }

    explicit Framework(const InitInfo& info) : Framework{ info, {} } {}
    explicit Framework(const InitInfo& info, const std::vector<std::string>& windowExtensions)
        : m_instance{ createInstance(info, windowExtensions) }
#if !defined(NDEBUG)
          ,
          m_debug{ m_instance }
#endif
    {
        m_logger.info("Vulkan framework initialized:\n\tAppName: {}\n\t{}\n\t{} {} {} {}",
                      info.appName,
                      info.engineName,
                      version::major,
                      version::minor,
                      version::patch,
                      version::tweak);
    }

    [[nodiscard]] auto getInstance() const -> vk::Instance {
        return *m_instance;
    }

private:
    [[nodiscard]] auto createInstance(const InitInfo& info, const std::vector<std::string>& windowExtensions) const
            -> vk::raii::Instance {
        dumpExtensions();
        dumpLayers();

        const auto enabledExtensions = mergeInstanceExtensions(defaultEnabledExtensions, windowExtensions);
        if (!validateExtension(enabledExtensions)) {
            throw std::runtime_error("All required extensions are not supported by device");
        }

        constexpr auto appVersion = vk::makeApiVersion(version::major, version::minor, version::patch, version::tweak);
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
                m_logger.warn("Validation layers are not available! Proceeding without them.");
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
        constexpr auto enabledValidationFeatures =
                std::array{ vk::ValidationFeatureEnableEXT::eSynchronizationValidation };
        const vk::StructureChain instanceCreateInfoChain(
                instanceCreateInfo,
                Debug::createDebugUtilsMessengerCreateInfo(),
                vk::ValidationFeaturesEXT{
                        .enabledValidationFeatureCount = static_cast<uint32_t>(enabledValidationFeatures.size()),
                        .pEnabledValidationFeatures = enabledValidationFeatures.data(),
                });
#else
        const vk::StructureChain instanceCreateInfoChain(instanceCreateInfo);
#endif
        return m_context.createInstance(instanceCreateInfoChain.get<vk::InstanceCreateInfo>());
    }

    void dumpExtensions() const {
        m_logger.info("Vulkan extensions:");
        for (const auto& extension : m_context.enumerateInstanceExtensionProperties()) {
            m_logger.info("\t{}", std::string_view(extension.extensionName));
        }
    }

    void dumpLayers() const {
        m_logger.info("Vulkan layers:");
        for (const auto& layer : m_context.enumerateInstanceLayerProperties()) {
            m_logger.info("\t{}", std::string_view(layer.layerName));
        }
    }

    static auto mergeInstanceExtensions(const std::span<const char* const> defaultExtensions,
                                        const std::span<const std::string>
                                                windowExtensions) -> std::vector<const char*> {
        std::vector mergedExtensions(defaultExtensions.begin(), defaultExtensions.end());
        if (windowExtensions.empty()) {
            return mergedExtensions;
        }
        for (const auto& extension : windowExtensions) {
            mergedExtensions.push_back(extension.c_str());
        }
        return mergedExtensions;
    }

    [[nodiscard]] auto validateExtension(std::span<const char* const> requestedExtensions) const -> bool {
        const auto extensionProperties = m_context.enumerateInstanceExtensionProperties()
                                         | std::views::transform([](const vk::ExtensionProperties& extension) {
                                               return std::string_view(extension.extensionName);
                                           });
        const auto requestedExtensionsRange = requestedExtensions | std::views::transform([](const auto& extension) {
                                                  return std::string_view(extension);
                                              });
        return core::arrayContainsArray(requestedExtensionsRange, extensionProperties);
    }

    [[nodiscard]] auto validateLayers(std::span<const char* const> requestedLayers) const -> bool {
        const auto layerPropertiesNames = m_context.enumerateInstanceLayerProperties()
                                          | std::views::transform([](const vk::LayerProperties& layer) {
                                                return std::string_view(layer.layerName);
                                            });
        const auto requestedLayersRange = requestedLayers | std::views::transform([](const auto& extension) {
                                              return std::string_view(extension);
                                          });
        return core::arrayContainsArray(requestedLayersRange, layerPropertiesNames);
    }

    vk::raii::Context m_context;
    vk::raii::Instance m_instance;

#if !defined(NDEBUG)
    Debug m_debug;
#endif
};
}// namespace th::render_system::vulkan
