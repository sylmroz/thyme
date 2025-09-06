module;

#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

export module th.core.logger;

import std;

namespace th {

export enum struct LogLevel {
    trace,
    debug,
    info,
    warn,
    error,
    critical,
    off
};

constexpr auto toSpdLogLevel(const LogLevel level) -> spdlog::level::level_enum {
    switch (level) {
        case LogLevel::trace: return spdlog::level::trace;
        case LogLevel::debug: return spdlog::level::debug;
        case LogLevel::info: return spdlog::level::info;
        case LogLevel::warn: return spdlog::level::warn;
        case LogLevel::error: return spdlog::level::err;
        case LogLevel::critical: return spdlog::level::critical;
        case LogLevel::off: return spdlog::level::off;
    }
    std::unreachable();
}

export template <class... Args>
struct basic_format_with_source_location {
    template <class Str>
        requires std::convertible_to<const Str&, std::format_string<Args...>>
    consteval basic_format_with_source_location(const Str fmt,
                                                const std::source_location& loc = std::source_location::current())
        : m_fmt(std::move(fmt)), location(loc.file_name(), static_cast<int>(loc.line()), loc.function_name()) {}

    [[nodiscard]] constexpr auto get_location() const {
        return location;
    }

    [[nodiscard]] constexpr auto get_format() const noexcept {
        return m_fmt;
    }

private:
    std::format_string<Args...> m_fmt;
    spdlog::source_loc location{};
};

template <class... _Args>
using format_with_source_location = basic_format_with_source_location<std::type_identity_t<_Args>...>;

export class Logger {
public:
    Logger(const LogLevel level, const std::string_view loggerName) {
        logger = spdlog::stdout_color_mt(std::string(loggerName));
        logger->set_pattern("%^[%T:%e][%n][%l][%@]: %v%$");
        const auto l = toSpdLogLevel(level);
        logger->set_level(l);
    }

    template <typename... Args>
    void trace(format_with_source_location<Args...> msg, Args&&... args) const noexcept {
        const auto fullMessage = std::format(msg.get_format(), std::forward<Args>(args)...);
        logger->log(msg.get_location(), spdlog::level::trace, fullMessage);
    }

    template <typename... Args>
    void debug(format_with_source_location<Args...> msg, Args&&... args) const noexcept {
        const auto fullMessage = std::format(msg.get_format(), std::forward<Args>(args)...);
        logger->log(msg.get_location(), spdlog::level::debug, fullMessage);
    }

    template <typename... Args>
    void info(format_with_source_location<Args...> msg, Args&&... args) const noexcept {
        const auto fullMessage = std::format(msg.get_format(), std::forward<Args>(args)...);
        logger->log(msg.get_location(), spdlog::level::info, fullMessage);
    }

    template <typename... Args>
    void warn(format_with_source_location<Args...> msg, Args&&... args) const noexcept {
        const auto fullMessage = std::format(msg.get_format(), std::forward<Args>(args)...);
        logger->log(msg.get_location(), spdlog::level::warn, fullMessage);
    }

    template <typename... Args>
    void error(format_with_source_location<Args...> msg, Args&&... args) const noexcept {
        const auto fullMessage = std::format(msg.get_format(), std::forward<Args>(args)...);
        logger->log(msg.get_location(), spdlog::level::err, fullMessage);
    }

    template <typename... Args>
    void critical(format_with_source_location<Args...> msg, Args&&... args) const noexcept {
        const auto fullMessage = std::format(msg.get_format(), std::forward<Args>(args)...);
        logger->log(msg.get_location(), spdlog::level::critical, fullMessage);
    }

    std::shared_ptr<spdlog::logger> logger;
};

export class ThymeLogger {
public:
    static void init(const LogLevel level) noexcept {
        s_logger = std::make_unique<Logger>(level, "ThymeApi");
    }

    static auto getLogger() -> Logger* {
        return s_logger.get();
    }

private:
    inline static std::unique_ptr<Logger> s_logger{ nullptr };
};

export class AppLogger {
public:
    static void init(const LogLevel level) noexcept {
        s_logger = std::make_unique<Logger>(level, "App");
    }

    static auto getLogger() -> Logger* {
        return s_logger.get();
    }

private:
    inline static std::unique_ptr<Logger> s_logger{ nullptr };
};

}// namespace th
