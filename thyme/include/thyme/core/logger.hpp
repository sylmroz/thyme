#pragma once

#include <thyme/export_macros.hpp>

#include <memory>
#include <string_view>

#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace Thyme {

class THYME_API Logger {
public:
    Logger(spdlog::level::level_enum consoleLoglevel, spdlog::level::level_enum fileLoglevel, std::string_view loggerName);

    std::unique_ptr<spdlog::logger> logger;
private:

};

class ThymeLogger {
public:
    static void init(spdlog::level::level_enum consoleLoglevel, spdlog::level::level_enum fileLoglevel) noexcept {
        s_logger = std::make_unique<Logger>(consoleLoglevel, fileLoglevel, "Thyme");
    }

    static Logger* getLogger() {
        return s_logger.get();
    }

private:
    inline static std::unique_ptr<Logger> s_logger{ nullptr };
};


#define TH_API_LOG_TRACE(...) SPDLOG_LOGGER_TRACE(::Thyme::ThymeLogger::getLogger()->logger, __VA_ARGS__);
#define TH_API_LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(::Thyme::ThymeLogger::getLogger()->logger, __VA_ARGS__);
#define TH_API_LOG_INFO(...) SPDLOG_LOGGER_INFO(::Thyme::ThymeLogger::getLogger()->logger, __VA_ARGS__);
#define TH_API_LOG_WARN(...) SPDLOG_LOGGER_WARN(::Thyme::ThymeLogger::getLogger()->logger, __VA_ARGS__);
#define TH_API_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(::Thyme::ThymeLogger::getLogger()->logger, __VA_ARGS__);
#define TH_API_LOG_CRITICA(...) SPDLOG_LOGGER_CRITICAL(::Thyme::ThymeLogger::getLogger()->logger, __VA_ARGS__);

class THYME_API AppLogger {
public:
    static void init(spdlog::level::level_enum consoleLoglevel, spdlog::level::level_enum fileLoglevel) noexcept {
        s_logger = std::make_unique<Logger>(consoleLoglevel, fileLoglevel, "App");
    }

    static Logger* getLogger() {
        return s_logger.get();
    }

private:
    inline static std::unique_ptr<Logger> s_logger{ nullptr };
};

#define TH_APP_LOG_TRACE(...) SPDLOG_LOGGER_TRACE(::Thyme::AppLogger::getLogger()->logger, __VA_ARGS__);
#define TH_APP_LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(::Thyme::AppLogger::getLogger()->logger, __VA_ARGS__);
#define TH_APP_LOG_INFO(...) SPDLOG_LOGGER_INFO(::Thyme::AppLogger::getLogger()->logger, __VA_ARGS__);
#define TH_APP_LOG_WARN(...) SPDLOG_LOGGER_WARN(::Thyme::AppLogger::getLogger()->logger, __VA_ARGS__);
#define TH_APP_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(::Thyme::AppLogger::getLogger()->logger, __VA_ARGS__);
#define TH_APP_LOG_CRITICA(...) SPDLOG_LOGGER_CRITICAL(::Thyme::AppLogger::getLogger()->logger, __VA_ARGS__);

}
