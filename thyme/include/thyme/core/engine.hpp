#pragma once

#include <thyme/export_macros.hpp>

#include <string>

#include <thyme/platform/vulkan/vulkan_layer.hpp>

namespace Thyme {

struct THYME_API EngineConfig {
    std::string engineName{ "Thyme" };
    std::string appName;
    uint32_t width{ 1920 };
    uint32_t height{ 1080 };
};

class THYME_API Engine final {
public:
    explicit Engine(const EngineConfig& engineConfig, Vulkan::VulkanLayerStack& layers);

    void run() const;

private:
    EngineConfig m_engineConfig;
    Vulkan::VulkanLayerStack& m_layers;
};

}// namespace Thyme