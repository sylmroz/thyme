#include <thyme/core/application.hpp>
#include <thyme/core/logger.hpp>

#include <variant>

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

#include <thyme/core/core.hpp>
#include <thyme/platform/vulkan/vulkan_layer.hpp>

class ExampleLayer final: public th::vulkan::VulkanOverlayLayer {
    struct MyEventDispatcher final: th::EventDispatcher<th::WindowResize, th::MousePosition> {
        void operator()(const th::WindowResize& event) override {
            // TH_APP_LOG_INFO("Example Layer::onEvent {}", event.toString());
        }
        void operator()(const th::MousePosition& event) override {
            // TH_APP_LOG_INFO("Example Layer::onEvent {}", event.toString());
        }
        using EventDispatcher::operator();
    };

public:
    explicit ExampleLayer() : OverlayLayer("example layer") {}
    void draw(vk::UniqueCommandBuffer&&) override {}
    void onAttach() override {}
    void onDetach() override {}
    void onEvent(const th::Event& event) override {
        std::visit(MyEventDispatcher{}, event);
    }
};

class ExampleApp: public th::Application {
public:
    ExampleApp() {
        layers.pushLayer(&exampleLayer);
    }

private:
    ExampleLayer exampleLayer;
};

int main() {
    th::AppLogger::init(spdlog::level::info);
    TH_APP_LOG_INFO("Hello from app");
    ExampleApp app;
    app.name = "AppThyme";
    app.run();

    return 0;
}