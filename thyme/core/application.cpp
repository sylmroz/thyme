module;

module th.core.application;

import nlohmann.json;

import th.core.events;
import th.platform.window;
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
        auto window_settings_events_dispatcher = WindowSettingsEventDispatcher(window_settings);
        auto window = GlfwWindow(window_settings.getConfig(), m_logger);
        window.subscribe([&window_settings_events_dispatcher](const Event& event) {
            window_settings_events_dispatcher(event);
        });

        auto engine = Engine(EngineConfig{ .app_name = m_name }, window, m_model_storage, m_logger);
        engine.run();
    } catch (const std::exception& e) {
        m_logger.error("Error occurred during app runtime\n Error: {}"sv, e.what());
    } catch (...) {
        m_logger.error("Unknown error occurred during app runtime"sv);
    }
}

}// namespace th
