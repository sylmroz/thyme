#include <thyme/core/application.hpp>
#include <thyme/core/engine.hpp>
#include <thyme/core/platform_context.hpp>
#include <thyme/platform/glfw_vulkan_platform_context.hpp>
#include <thyme/platform/imgui_context.hpp>
#include <thyme/platform/vulkan/vulkan_layer.hpp>

#include <spdlog/spdlog.h>

import th.core.logger;
import th.platform.glfw.glfw_context;

namespace th {

using namespace std::string_view_literals;

template <typename... Context>
// requires(std::is_base_of_v<PlatformContext, Context>)
auto createEngine(const EngineConfig& config, vulkan::VulkanLayerStack& layers, scene::ModelStorage& modelStorage)
        -> Engine {
    [[maybe_unused]] static std::tuple<Context...> ctx;
    return Engine(config, layers, modelStorage);
}

Application::Application() {
    ThymeLogger::init(LogLevel::trace);
    core::ThymeLogger::init(core::LogLevel::trace);
}

void Application::run() {
    const auto logger = core::ThymeLogger::getLogger();
    logger->info("Starting Thyme api {}", name);
    try {
        auto engine = createEngine<platform::glfw::GlfwVulkanPlatformContext, ImGuiContext>(
                EngineConfig{ .appName = name }, layers, modelStorage);
        engine.run();
    } catch (const std::exception& e) {
        constexpr auto error_format = "Error occurred during app runtime\n Error: {}"sv;
        logger->error(error_format, e.what());
    } catch (...) {
        logger->error("Unknown error occurred during app runtime"sv);
    }
}

}// namespace th
