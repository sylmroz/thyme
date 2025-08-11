module;

#include "thyme/platform/glfw_window.hpp"

export module th.render_system.framework;

import th.core.logger;

//import th.render_system.vulkan.framework;

namespace th::render_system {

export enum struct BackendType {
    vulkan
};

export class Framework {
public:
    struct InitInfo {
        std::string appName;
        std::string engineName;
    };
    Framework() : m_logger{ *core::ThymeLogger::getLogger() } {}
    explicit Framework(core::Logger& logger) : m_logger{ logger } {}

    Framework(const Framework&) = default;
    Framework(Framework&&) = default;
    Framework& operator=(const Framework&) = delete;
    Framework& operator=(Framework&&) = delete;

    virtual ~Framework() = default;

protected:
    core::Logger& m_logger;
};

}// namespace th::render_system
