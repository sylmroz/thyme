module;

#include <spdlog/spdlog.h>
#include "thyme/core/logger.hpp"

module thyme.core.application;

import thyme.core.engine;
import thyme.core.platform_context;
import thyme.platform.gflw_vulkan_platform_context;

template<typename Context>
    requires(std::is_base_of_v<Thyme::PlatformContext, Context>)
Thyme::Engine createEngine(const Thyme::EngineConfig& config) {
    [[maybe_unused]] static auto ctx = Context();
    return Thyme::Engine(config);
}

Thyme::Application::Application() {
    ThymeLogger::init(spdlog::level::trace);
}

void Thyme::Application::run() const {
    TH_API_LOG_INFO("Start {} app", name);
    try {
        const auto engine = createEngine<GlfwVulkanPlatformContext>(EngineConfig{ .appName = name });
        engine.run();
    } catch (const std::exception& e) {
        TH_API_LOG_ERROR(e.what());
    } catch (...) {
        TH_API_LOG_ERROR("Unknown exception thrown");
    }
}
