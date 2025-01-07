module;

#include <thyme/export_macros.hpp>

#include <string>

export module thyme.core.engine;
import thyme.platform.vulkan_layer;

namespace Thyme {

export struct EngineConfig {
    std::string engineName{ "Thyme" };
    std::string appName;
    uint32_t width{ 1920 };
    uint32_t height{ 1080 };
};

export class THYME_API Engine final {
public:
    explicit Engine(const EngineConfig& engineConfig, Vulkan::VulkanLayerStack& layers);

    void run() const;

private:
    EngineConfig m_engineConfig;
    Vulkan::VulkanLayerStack& m_layers;
};

}// namespace Thyme