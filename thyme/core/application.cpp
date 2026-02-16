module;

module th.core.application;

import glm;
import nlohmann.json;

import th.core.events;
import th.platform.window;
import th.platform.window_event_handler;
import th.platform.window_settings;
import th.platform.glfw.glfw_window;

namespace th {

using namespace std::string_view_literals;

Application::Application(Logger& logger) : m_model_storage{ logger }, m_logger{ logger } {}

void Application::run() {
    m_logger.info("Starting Thyme api {}"sv, m_name);
    try {
        auto window_settings =
                WindowSettings(WindowConfig{ .width = 1280, .height = 720, .name = "Thyme", .maximized = false });
        auto window_event_handlers = WindowEventsHandlers();
        auto window = GlfwWindow(window_settings.getConfig(), window_event_handlers, m_logger);
        auto camera =
                Camera{ CameraArguments{ .fov = 45.0f,
                                         .znear = 0.1f,
                                         .zfar = 100.0f,
                                         .resolution = window.getFrameBufferSize(),
                                         .eye = { 2.0f, 2.0f, 2.0f },
                                         .center = { 0.0f, 0.0f, 0.0f },
                                         .up = { 0.0f, 0.0f, 1.0f },
                                         .yaw_pitch_roll = YawPitchRoll{ .yaw = 0.0f, .pitch = 0.0f, .roll = 0.0f } } };

        window_event_handlers.addEventListener<WindowResizedEvent>(
                [&window_settings, &camera](const WindowResizedEvent& window_resize) {
                    const auto [width, height] = window_resize;
                    window_settings.setResolution(glm::uvec2(width, height));
                    camera.setResolution(glm::vec2{ static_cast<float>(width), static_cast<float>(height) });
                });

        window_event_handlers.addEventListener<WindowMaximizedEvent>(
                [&window_settings](const WindowMaximizedEvent maximize) {
                    window_settings.setMaximized(maximize.maximized);
                });

        auto engine =
                Engine(EngineConfig{ .app_name = m_name }, window, m_model_storage, window_event_handlers, m_logger);
        engine.run(camera);
    } catch (const std::exception& e) {
        m_logger.error("Error occurred during app runtime\n Error: {}"sv, e.what());
    } catch (...) {
        m_logger.error("Unknown error occurred during app runtime"sv);
    }
}

}// namespace th
