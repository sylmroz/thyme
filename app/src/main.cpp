#include <thyme/core/logger.hpp>
#include <variant>

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

import thyme.core;
import thyme.platform.vulkan_layer;

class ExampleLayer final: public Thyme::Vulkan::VulkanOverlayLayer {
    struct MyEventDispatcher final: Thyme::EventDispatcher<Thyme::WindowResize, Thyme::MousePosition> {
        void operator()(const Thyme::WindowResize& event) override {
            // TH_APP_LOG_INFO("Example Layer::onEvent {}", event.toString());
        }
        void operator()(const Thyme::MousePosition& event) override {
            // TH_APP_LOG_INFO("Example Layer::onEvent {}", event.toString());
        }
        using EventDispatcher::operator();
    };

public:
    explicit ExampleLayer() : OverlayLayer("example layer") {}
    void draw(vk::UniqueCommandBuffer&&) override {}
    void onAttach() override {}
    void onDetach() override {}
    void onEvent(const Thyme::Event& event) override {
        std::visit(MyEventDispatcher{}, event);
    }
};

class ExampleApp: public Thyme::Application {
public:
    ExampleApp() {
        layers.pushLayer(&exampleLayer);
    }

private:
    ExampleLayer exampleLayer;
};

int main() {
    Thyme::AppLogger::init(spdlog::level::info);
    TH_APP_LOG_INFO("Hello from app");
    ExampleApp app;
    app.name = "AppThyme";
    app.run();

    return 0;
}