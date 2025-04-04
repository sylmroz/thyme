#include <thyme/core/application.hpp>
#include <thyme/core/engine.hpp>
#include <thyme/core/logger.hpp>
#include <thyme/core/platform_context.hpp>
#include <thyme/platform/glfw_vulkan_platform_context.hpp>
#include <thyme/platform/imgui_context.hpp>
#include <thyme/platform/vulkan/vulkan_layer.hpp>

#include <spdlog/spdlog.h>

namespace th {

template <typename... Context>
// requires(std::is_base_of_v<PlatformContext, Context>)
Engine createEngine(const EngineConfig& config, vulkan::VulkanLayerStack& layers, scene::ModelStorage& modelStorage) {
    [[maybe_unused]] static std::tuple<Context...> ctx;
    return Engine(config, layers, modelStorage);
}

Application::Application() {
    ThymeLogger::init(spdlog::level::trace);
}

void Application::run() {
    TH_API_LOG_INFO("Start {} app", name);
    try {
        auto engine = createEngine<GlfwVulkanPlatformContext, ImGuiContext>(
                EngineConfig{ .appName = name }, layers, modelStorage);
        engine.run();
    } catch (const std::exception& e) {
        TH_API_LOG_ERROR(e.what());
    } catch (...) {
        TH_API_LOG_ERROR("Unknown exception thrown");
    }
}

}// namespace th
