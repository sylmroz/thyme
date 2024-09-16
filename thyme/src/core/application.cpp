#include "thyme/core/application.hpp"

#include "thyme/core/logger.hpp"
#include "thyme/platform/glfw_vulkan_platform_context.hpp"

using namespace Thyme;

template<typename Context>
    requires(std::is_base_of<PlatformContext, Context>::value)
std::unique_ptr<Engine> createEngine(const EngineConfig& config) {
    class ContextualizedEngine: public Engine {
    public:
        ContextualizedEngine(const EngineConfig& config) : Engine{ config } {}

    private:
        Context context{};
    };
    return std::make_unique<ContextualizedEngine>(config);
}

Thyme::Application::Application() {
    ThymeLogger::init(spdlog::level::trace);
}

void Thyme::Application::run() {
    TH_API_LOG_INFO("Start {} app", name);
    auto engine = createEngine<GlfwVulkanPlatformContext>(EngineConfig{ .appName = name });
    engine->run();
}
