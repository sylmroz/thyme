#include <thyme/core/logger.hpp>
#include <variant>

#include <spdlog/spdlog.h>

import thyme.core;

class ExampleLayer final: public Thyme::OverlayLayer {
    struct MyEventDispatcher final: Thyme::EventDispatcher<Thyme::WindowResize> {
        void operator()(const Thyme::WindowResize& event) override {
            TH_APP_LOG_INFO("Example Layer::onEvent {}", event.toString());
        }
        using EventDispatcher::operator();
    };

public:
    explicit ExampleLayer(Thyme::LayerStack<Layer>& layers) : OverlayLayer("example layer", layers) {}
    void draw() override {}
    void onAttach() override {}
    void onDetach() override {}
    void onEvent(const Thyme::Event& event) override {
        std::visit(MyEventDispatcher{}, event);
    }
};

class ExampleApp: public Thyme::Application {
public:
    ExampleApp() : exampleLayer(addLayer<ExampleLayer>()) {}

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