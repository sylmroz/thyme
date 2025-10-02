module;

module th.core.application;

import nlohmann.json;

import th.platform.window;
import th.platform.glfw.glfw_window;

namespace th {

using namespace std::string_view_literals;

Application::Application(Logger& logger) : modelStorage{ logger }, m_logger{ logger } {}

void Application::run() {
    m_logger.info("Starting Thyme api {}"sv, name);
    try {
        auto window = GlfwWindow(WindowConfig{ .width = 1280, .height = 720, .name = "Thyme", .maximalized = false },
                                 m_logger);
        auto engine = Engine(EngineConfig{ .appName = name }, window, modelStorage, m_logger);
        engine.run();
    } catch (const std::exception& e) {
        m_logger.error("Error occurred during app runtime\n Error: {}"sv, e.what());
    } catch (...) {
        m_logger.error("Unknown error occurred during app runtime"sv);
    }
}

}// namespace th
