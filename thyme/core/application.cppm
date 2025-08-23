module;

#include <spdlog/spdlog.h>

#include <string>

export module th.core.application;

import th.core.engine;
import th.core.logger;
import th.platform.glfw.glfw_context;
import th.scene.model;
import th.platform.imgui_context;

namespace th::core {
export class Application {
public:
    Application();
    std::string name{ "Thyme" };
    void run();

    scene::ModelStorage modelStorage;
};

}// namespace th::core

namespace th::core {

using namespace std::string_view_literals;

template <typename... Context>
// requires(std::is_base_of_v<PlatformContext, Context>)
auto createEngine(const EngineConfig& config, scene::ModelStorage& modelStorage)
        -> Engine {
    [[maybe_unused]] static std::tuple<Context...> ctx;
    return Engine(config, modelStorage);
}

Application::Application() {
    ThymeLogger::init(LogLevel::trace);
}

void Application::run() {
    const auto logger = ThymeLogger::getLogger();
    logger->info("Starting Thyme api {}"sv, name);
    try {
        auto engine = createEngine<ImGuiContext>(EngineConfig{ .appName = name }, modelStorage);
        engine.run();
    } catch (const std::exception& e) {
        logger->error("Error occurred during app runtime\n Error: {}"sv, e.what());
    } catch (...) {
        logger->error("Unknown error occurred during app runtime"sv);
    }
}

}// namespace th::core
