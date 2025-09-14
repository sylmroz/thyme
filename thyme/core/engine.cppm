module;

#include <functional>
#include <string>
#include <variant>

#include <imgui.h>

export module th.core.engine;

import th.render_system.vulkan;

import th.platform.glfw.glfw_window;
import th.platform.window;
import th.platform.glfw.glfw_context;

import th.core.events;
import th.core.logger;

import th.scene.model;
import th.scene.camera;

import vulkan_hpp;

export namespace th {

enum struct BackendType {
    Vulkan
};

struct EngineConfig {
    uint32_t width{ 1920 };
    uint32_t height{ 1080 };
    BackendType backend{ BackendType::Vulkan };
    std::string engineName{ "Thyme" };
    std::string appName;
};

class Engine final {
public:
    explicit Engine(const EngineConfig& engineConfig, ModelStorage& modelStorage, Logger& logger);

    void run();

private:
    EngineConfig m_engineConfig;
    Camera m_camera;
    ModelStorage& m_modelStorage;
    Logger& m_logger;
};
}// namespace th

