export module th.core.engine;

import std;

import vulkan_hpp;

import th.render_system.vulkan;
import th.platform.glfw.glfw_window;
import th.platform.window;
import th.core.events;
import th.core.logger;
import th.scene.model;
import th.scene.camera;

export namespace th {

enum struct BackendType {
    Vulkan
};

struct EngineConfig {
    BackendType backend{ BackendType::Vulkan };
    std::string engineName{ "Thyme" };
    std::string appName;
};

class Engine final {
public:
    explicit Engine(const EngineConfig& engineConfig, GlfwWindow& window, ModelStorage& modelStorage, Logger& logger);

    void run();

private:
    EngineConfig m_engineConfig;
    Camera m_camera;
    GlfwWindow& m_window;

    ModelStorage& m_modelStorage;
    Logger& m_logger;
};
}// namespace th

