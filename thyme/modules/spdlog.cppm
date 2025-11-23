module;

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

export module spdlog;

export namespace spdlog {
namespace level {
    using level::level_enum;
    using level_enum::trace;
    using level_enum::debug;
    using level_enum::info;
    using level_enum::warn;
    using level_enum::err;
    using level_enum::critical;
    using level_enum::off;
}// namespace level
using ::spdlog::source_loc;
using ::spdlog::stdout_color_mt;
using ::spdlog::logger;
}// namespace spdlog
