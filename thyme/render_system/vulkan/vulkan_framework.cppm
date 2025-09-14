module;

//#include "thyme/version.hpp"

#include <string>

export module th.render_system.vulkan:framework;

#if !defined(NDEBUG)
import :debug;
#endif

import th.core.logger;
import th.core.utils;

import vulkan_hpp;

namespace th {

template <typename ExtensionProvider>
concept has_get_extension_static_function = requires() {
    { ExtensionProvider::getExtensions() } -> std::same_as<std::vector<std::string>>;
};

template <typename ExtensionProvider>
concept CanPassWindow = has_get_extension_static_function<ExtensionProvider> || std::same_as<ExtensionProvider, void>;

export class VulkanFramework final {
public:
    struct InitInfo {
        std::string appName;
        std::string engineName;
    };
    template <CanPassWindow window = void>
    static auto create(const InitInfo& info, Logger& logger) -> VulkanFramework {
        if constexpr (std::is_same_v<window, void>) {
            return VulkanFramework(info, logger);
        } else {
            return VulkanFramework(info, window::getExtensions(), logger);
        }
    }

    explicit VulkanFramework(const InitInfo& info, Logger& logger);
    explicit VulkanFramework(const InitInfo& info, const std::vector<std::string>& windowExtensions, Logger& logger);

    [[nodiscard]] auto getInstance() const -> const vk::raii::Instance& {
        return m_instance;
    }

private:
    [[nodiscard]] auto createInstance(const InitInfo& info, const std::vector<std::string>& windowExtensions) const
            -> vk::raii::Instance;

    void dumpExtensions() const;

    void dumpLayers() const;

    static auto mergeInstanceExtensions(std::span<const char* const> defaultExtensions,
                                                      std::span<const std::string>
                                                              windowExtensions) -> std::vector<const char*>;

    [[nodiscard]] auto validateExtension(std::span<const char* const> requestedExtensions) const -> bool;

    [[nodiscard]] auto validateLayers(std::span<const char* const> requestedLayers) const -> bool;

    Logger& m_logger;

    vk::raii::Context m_context;
    vk::raii::Instance m_instance;

#if !defined(NDEBUG)
    VulkanDebug m_debug;
#endif
};

}// namespace th

