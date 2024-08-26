#include <core/logger.hpp>
#include <core/application.hpp>

#include <iostream>

int main() {
    Thyme::AppLogger::init(spdlog::level::info, spdlog::level::off);
    TH_APP_LOG_INFO("Hello from app");
    Thyme::Application app;
    app.name = "AppThyme";
    app.run();

    return 0;
}