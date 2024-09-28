import thyme.core.application;

#include <thyme/core/logger.hpp>

#include <spdlog/spdlog.h>

int main() {
    Thyme::AppLogger::init(spdlog::level::info);
    TH_APP_LOG_INFO("Hello from app");
    Thyme::Application app;
    app.name = "AppThyme";
    app.run();

    return 0;
}