module;

#include <spdlog/spdlog.h>
#include "thyme/core/logger.hpp"

module thyme.core.application;

import thyme.core.engine;
import thyme.core.platform_context;
import thyme.platform.gflw_vulkan_platform_context;
import thyme.platform.vulkan_layer;

using namespace Thyme;

template<typename Context>
    requires(std::is_base_of_v<PlatformContext, Context>)
Engine createEngine(const EngineConfig& config, Vulkan::VulkanLayerStack& layers) {
    [[maybe_unused]] static auto ctx = Context();
    return Engine(config, layers);
}

Application::Application() {
    ThymeLogger::init(spdlog::level::trace);
}

void Application::run() {
    TH_API_LOG_INFO("Start {} app", name);
    try {
        const auto engine = createEngine<GlfwVulkanPlatformContext>(EngineConfig{ .appName = name }, layers);
        engine.run();
    } catch (const std::exception& e) {
        TH_API_LOG_ERROR(e.what());
    } catch (...) {
        TH_API_LOG_ERROR("Unknown exception thrown");
    }
}
