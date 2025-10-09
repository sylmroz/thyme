export module th.render_system.vulkan:framework;

import std;

import vulkan_hpp;

import th.core.logger;
import th.core.utils;

#if !defined(NDEBUG)
import :debug;
#endif

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
        std::string app_name;
        std::string engine_name;
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
    explicit VulkanFramework(const InitInfo& info, const std::vector<std::string>& window_extensions, Logger& logger);

    [[nodiscard]] auto getInstance() const -> const vk::raii::Instance& {
        return m_instance;
    }

private:
    [[nodiscard]] auto createInstance(const InitInfo& info, const std::vector<std::string>& window_extensions) const
            -> vk::raii::Instance;

    void dumpExtensions() const;

    void dumpLayers() const;

    static auto mergeInstanceExtensions(std::span<const char* const> default_extensions,
                                                      std::span<const std::string>
                                                              window_extensions) -> std::vector<const char*>;

    [[nodiscard]] auto validateExtension(std::span<const char* const> requested_extensions) const -> bool;

    [[nodiscard]] auto validateLayers(std::span<const char* const> requested_layers) const -> bool;

    Logger& m_logger;

    vk::raii::Context m_context;
    vk::raii::Instance m_instance;

#if !defined(NDEBUG)
    VulkanDebug m_debug;
#endif
};

}// namespace th

