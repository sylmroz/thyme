#include "core/application.hpp"

#include "core/logger.hpp"

Thyme::Application::Application() {
    ThymeLogger::init(spdlog::level::info, spdlog::level::off);
}

void Thyme::Application::run() {
    TH_API_LOG_INFO("Start {} app", name);
    engine.run();
}
