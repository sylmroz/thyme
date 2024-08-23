#include <lib.hpp>

#include <logger.hpp>

#include <iostream>

int main() {
    Thyme::AppLogger::init(spdlog::level::info, spdlog::level::off);
    TH_APP_LOG_INFO(Thyme::getPi());

    return 0;
}