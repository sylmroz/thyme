#pragma once

#include <thyme/export_macros.hpp>
#include <thyme/platform/vulkan/vulkan_layer.hpp>
#include <thyme/scene/camera.hpp>
#include <thyme/scene/model.hpp>

#include <string>

namespace th {

struct THYME_API EngineConfig {
    std::string engineName{ "Thyme" };
    std::string appName;
    uint32_t width{ 1920 };
    uint32_t height{ 1080 };
};

class THYME_API Engine final {
public:
    explicit Engine(const EngineConfig& engineConfig, vulkan::VulkanLayerStack& layers,
                    scene::ModelStorage& modelStorage);

    void run();

private:
    EngineConfig m_engineConfig;
    scene::Camera m_camera;
    vulkan::VulkanLayerStack& m_layers;
    scene::ModelStorage& m_modelStorage;
};

}// namespace th