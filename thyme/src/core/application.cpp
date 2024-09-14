#include "thyme/core/application.hpp"

#include "thyme/core/logger.hpp"
#include "thyme/platform/glfw_vulkan_platform_context.hpp"

Thyme::Application::Application() {
    ThymeLogger::init(spdlog::level::trace);
}

void Thyme::Application::run() {
    TH_API_LOG_INFO("Start {} app", name);
    GlfwVulkanPlatformContext context;
    auto engine = Engine(EngineConfig{ .appName = name }, context);
    engine.run();
}
