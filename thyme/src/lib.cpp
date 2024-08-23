#include <lib.hpp>

#include <logger.hpp>

#include <iostream>

double Thyme::getPi() {
    ThymeLogger::init(spdlog::level::info, spdlog::level::off);
    TH_API_LOG_INFO("hey! I am here! :)");

    return 3.14;
}
