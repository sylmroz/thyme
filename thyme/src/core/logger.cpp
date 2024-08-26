#include "core/logger.hpp"

Thyme::Logger::Logger(spdlog::level::level_enum consoleLoglevel, spdlog::level::level_enum fileLoglevel, std::string_view loggerName) {
    auto consoleLogger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleLogger->set_level(consoleLoglevel);
    auto fileLogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(fmt::format("logs/{}.txt", loggerName), true);
    fileLogger->set_level(fileLoglevel);

    logger = std::make_unique<spdlog::logger>(spdlog::logger(std::string(loggerName), { consoleLogger, fileLogger }));
    logger->set_pattern("%^[%T:%e] [%n] [%l] [%@]: %v%$");
}