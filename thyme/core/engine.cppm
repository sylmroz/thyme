export module th.core.engine;

import std;

import vulkan_hpp;

import th.render_system.vulkan;
import th.platform.glfw.glfw_window;
import th.platform.window;
import th.platform.window_event_handler;
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
    std::string engine_name{ "Thyme" };
    std::string app_name;
};

class Engine final {
public:
    explicit Engine(const EngineConfig& engine_config, GlfwWindow& window, ModelStorage& model_storage, WindowEventsHandlers& window_event_handler, Logger& logger);

    void run();

private:
    EngineConfig m_engine_config;
    Camera m_camera;
    GlfwWindow& m_window;
    WindowEventsHandlers& m_window_event_handler;

    ModelStorage& m_model_storage;
    Logger& m_logger;
};
}// namespace th
