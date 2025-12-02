export module th.core.logger;

import std;

#ifndef LOGGER_USE_STD_PRINT
import spdlog;
#endif

namespace th {

export enum struct LogLevel {
    trace,
    debug,
    info,
    warn,
    err,
    critical,
    off
};

#ifndef LOGGER_USE_STD_PRINT
constexpr auto toSpdLogLevel(const LogLevel level) -> spdlog::level::level_enum {
    switch (level) {
        case LogLevel::trace: return spdlog::level::trace;
        case LogLevel::debug: return spdlog::level::debug;
        case LogLevel::info: return spdlog::level::info;
        case LogLevel::warn: return spdlog::level::warn;
        case LogLevel::err: return spdlog::level::err;
        case LogLevel::critical: return spdlog::level::critical;
        case LogLevel::off: return spdlog::level::off;
    }
    std::unreachable();
}
#else
constexpr auto toString(const LogLevel level) -> std::string_view {
    switch (level) {
        case LogLevel::trace: return "trace";
        case LogLevel::debug: return "debug";
        case LogLevel::info: return "info";
        case LogLevel::warn: return "warn";
        case LogLevel::err: return "err";
        case LogLevel::critical: return "critical";
        case LogLevel::off: return "off";
    }
    std::unreachable();
}
#endif

export template <class... Args>
struct basic_format_with_source_location {
    template <class Str>
        requires std::convertible_to<const Str&, std::format_string<Args...>>
    consteval basic_format_with_source_location(const Str fmt,
                                                const std::source_location& loc = std::source_location::current())
        : m_fmt(std::move(fmt)),
#ifndef LOGGER_USE_STD_PRINT
          location(loc.file_name(), static_cast<int>(loc.line()), loc.function_name())
#else
          location(loc)
#endif
    {
    }

    [[nodiscard]] constexpr auto get_location() const {
        return location;
    }

    [[nodiscard]] constexpr auto get_format() const noexcept {
        return m_fmt;
    }

private:
    std::format_string<Args...> m_fmt;
#ifndef LOGGER_USE_STD_PRINT
    spdlog::source_loc location{};
#else
    std::source_location location{};
#endif
};

template <class... _Args>
using format_with_source_location = basic_format_with_source_location<std::type_identity_t<_Args>...>;

export class Logger {
public:
    Logger(const LogLevel level, const std::string_view logger_name) {
#ifndef LOGGER_USE_STD_PRINT
        logger = spdlog::stdout_color_mt(std::string(logger_name));
        logger->set_pattern("%^[%T:%e][%n][%l][%@]: %v%$");
        const auto l = toSpdLogLevel(level);
        logger->set_level(l);
#else
        m_current_log_level = level;
        m_logger_name = logger_name;
#endif
    }

    template <typename... Args>
    void trace(format_with_source_location<Args...> msg, Args&&... args) const noexcept {
        const auto full_message = std::format(msg.get_format(), std::forward<Args>(args)...);
#ifndef LOGGER_USE_STD_PRINT
        logger->log(msg.get_location(), spdlog::level::trace, full_message);
#else
        printMessage(msg.get_location(), LogLevel::trace, full_message);
#endif
    }

    template <typename... Args>
    void debug(format_with_source_location<Args...> msg, Args&&... args) const noexcept {
        const auto full_message = std::format(msg.get_format(), std::forward<Args>(args)...);
#ifndef LOGGER_USE_STD_PRINT
        logger->log(msg.get_location(), spdlog::level::debug, full_message);
#else
        printMessage(msg.get_location(), LogLevel::debug, full_message);
#endif
    }

    template <typename... Args>
    void info(format_with_source_location<Args...> msg, Args&&... args) const noexcept {
        const auto full_message = std::format(msg.get_format(), std::forward<Args>(args)...);
#ifndef LOGGER_USE_STD_PRINT
        logger->log(msg.get_location(), spdlog::level::info, full_message);
#else
        printMessage(msg.get_location(), LogLevel::info, full_message);
#endif
    }

    template <typename... Args>
    void warn(format_with_source_location<Args...> msg, Args&&... args) const noexcept {
        const auto full_message = std::format(msg.get_format(), std::forward<Args>(args)...);
#ifndef LOGGER_USE_STD_PRINT
        logger->log(msg.get_location(), spdlog::level::warn, full_message);
#else
        printMessage(msg.get_location(), LogLevel::warn, full_message);
#endif
    }

    template <typename... Args>
    void error(format_with_source_location<Args...> msg, Args&&... args) const noexcept {
        const auto full_message = std::format(msg.get_format(), std::forward<Args>(args)...);
#ifndef LOGGER_USE_STD_PRINT
        logger->log(msg.get_location(), spdlog::level::err, full_message);
#else
        printMessage(msg.get_location(), LogLevel::err, full_message);
#endif
    }

    template <typename... Args>
    void critical(format_with_source_location<Args...> msg, Args&&... args) const noexcept {
        const auto full_message = std::format(msg.get_format(), std::forward<Args>(args)...);
#ifndef LOGGER_USE_STD_PRINT
        logger->log(msg.get_location(), spdlog::level::critical, full_message);
#else
        printMessage(msg.get_location(), LogLevel::critical, full_message);
#endif
    }

private:
#ifndef LOGGER_USE_STD_PRINT
    std::shared_ptr<spdlog::logger> logger;
#else
    LogLevel m_current_log_level;
    std::string m_logger_name;
    void printMessage(const std::source_location& location, const LogLevel level, const std::string_view message) const {
        if (m_current_log_level <= level) {
            const auto now = std::chrono::system_clock::now();
            std::println("[{}][{}][{}][{}]: {}",
                         now,
                         m_logger_name,
                         toString(level),
                         std::format("{}:{}", location.file_name(), location.line()),
                         message);
        }
    }
#endif
};

}// namespace th
