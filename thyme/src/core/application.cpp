#include "thyme/core/application.hpp"

#include "thyme/core/logger.hpp"

Thyme::Application::Application() {
    ThymeLogger::init(spdlog::level::trace);
}

void Thyme::Application::run() {
    TH_API_LOG_INFO("Start {} app", name);
    auto engine = Engine(EngineConfig{ .appName = name });
    engine.run();
}
