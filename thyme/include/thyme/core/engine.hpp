#pragma once

#include <thyme/export_macros.hpp>

#include <string>

#include <thyme/platform/vulkan/vulkan_layer.hpp>

namespace th {

struct THYME_API EngineConfig {
    std::string engineName{ "Thyme" };
    std::string appName;
    uint32_t width{ 1920 };
    uint32_t height{ 1080 };
};

class THYME_API Engine final {
public:
    explicit Engine(const EngineConfig& engineConfig, vulkan::VulkanLayerStack& layers);

    void run() const;

private:
    EngineConfig m_engineConfig;
    vulkan::VulkanLayerStack& m_layers;
};

}// namespace Thyme