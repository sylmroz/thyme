#include <thyme/core/application.hpp>

#include <variant>

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

#include <thyme/core/core.hpp>
#include <thyme/platform/vulkan/vulkan_layer.hpp>

import th.core.logger;

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
    void draw(vk::CommandBuffer) override {}
    void onAttach() override {}
    void onDetach() override {}
    void onEvent(const th::Event& event) override {
        std::visit(MyEventDispatcher{}, event);
    }
    void start() override {}
    void submit() override {}
};

class ExampleApp: public th::Application {
public:
    ExampleApp() {
        layers.pushLayer(&exampleLayer);
        modelStorage.addModel(th::scene::Model{
                .name = "Grumpy 1",
                .mesh =
                        th::scene::Mesh{
                                .vertices = { { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                                              { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
                                              { { 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
                                              { { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } } },
                                .indices = { 0, 1, 2, 2, 3, 0 } },
                .texture = th::TextureData("C:\\Users\\sylwek\\Desktop\\grumpy.jpg"),
                .onAnimate = [](th::scene::Model& model) {
                    using namespace std::chrono;
                    static const auto startTime = high_resolution_clock::now();
                    const auto currentTime = high_resolution_clock::now();
                    const auto deltaTime = duration<float, seconds::period>(currentTime - startTime).count();
                    model.transformation.rotate(deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                } });
        modelStorage.addModel(th::scene::Model{
                .name = "Grumpy 2",
                .mesh =
                        th::scene::Mesh{
                                .vertices = { { { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                                              { { 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
                                              { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
                                              { { -0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } } },
                                .indices = { 0, 1, 2, 2, 3, 0 } },
                .texture = th::TextureData("C:\\Users\\sylwek\\Desktop\\grumpy2.jpg"),
                .onAnimate = [](th::scene::Model& model) {
                    using namespace std::chrono;
                    static const auto startTime = high_resolution_clock::now();
                    const auto currentTime = high_resolution_clock::now();
                    const auto deltaTime = duration<float, seconds::period>(currentTime - startTime).count();
                    model.transformation.rotate(-deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                } });
    }

private:
    ExampleLayer exampleLayer;
};

int main() {
    th::core::AppLogger::init(th::core::LogLevel::info);
    th::core::AppLogger::getLogger()->info("Hello from app");
    ExampleApp app;
    app.name = "AppThyme";
    app.run();

    return 0;
}