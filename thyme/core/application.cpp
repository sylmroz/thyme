module;

module th.core.application;

namespace th {

using namespace std::string_view_literals;

auto createEngine(const EngineConfig& config, ModelStorage& modelStorage, Logger& logger) -> Engine {
    [[maybe_unused]] static ImGuiContext im_gui_context;
    return Engine(config, modelStorage, logger);
}

Application::Application(Logger& logger) : modelStorage{ logger }, m_logger{ logger } {}

void Application::run() {
    m_logger.info("Starting Thyme api {}"sv, name);
    try {
        auto engine = createEngine(EngineConfig{ .appName = name }, modelStorage, m_logger);
        engine.run();
    } catch (const std::exception& e) {
        m_logger.error("Error occurred during app runtime\n Error: {}"sv, e.what());
    } catch (...) {
        m_logger.error("Unknown error occurred during app runtime"sv);
    }
}

}// namespace th
