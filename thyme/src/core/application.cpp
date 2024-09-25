#include "thyme/core/application.hpp"

#include "thyme/core/engine.hpp"
#include "thyme/core/logger.hpp"
#include "thyme/platform/glfw_vulkan_platform_context.hpp"

template<typename Context>
    requires(std::is_base_of_v<Thyme::PlatformContext, Context>)
std::unique_ptr<Thyme::Engine> createEngine(const Thyme::EngineConfig& config) {
    class ContextualizedEngine: public Thyme::Engine {
    public:
        explicit ContextualizedEngine(const Thyme::EngineConfig& config) : Thyme::Engine{ config } {}

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
    try {
        const auto engine = createEngine<GlfwVulkanPlatformContext>(EngineConfig{ .appName = name });
        engine->run();
    } catch (const std::exception& e) {
        TH_API_LOG_ERROR(e.what());
    } catch (...) {
        TH_API_LOG_ERROR("Unknown exception thrown");
    }
}
