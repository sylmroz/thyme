import std;
import glm;

import th.core.logger;
import th.core.application;

import th.scene.model;
import th.scene.texture_data;

import th.platform.window;
import th.platform.glfw.glfw_window;

class ExampleApp: public th::Application {
public:
    explicit ExampleApp(th::Logger& logger): th::Application{logger} {
        m_model_storage.addModel(th::Model{
                .name = "Grumpy 1",
                .mesh =
                        th::Mesh{
                                .vertices = { { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                                              { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
                                              { { 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
                                              { { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } } },
                                .indices = { 0, 1, 2, 2, 3, 0 } },
                .texture = th::TextureData(std::filesystem::current_path() / "assets" / "grumpy.jpg"),
                .on_animate = [](th::Model& model) {
                    using namespace std::chrono;
                    static const auto startTime = high_resolution_clock::now();
                    const auto currentTime = high_resolution_clock::now();
                    const auto deltaTime = duration<float, seconds::period>(currentTime - startTime).count();
                    model.transformation.rotate(deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                } });
        m_model_storage.addModel(th::Model{
                .name = "Grumpy 2",
                .mesh =
                        th::Mesh{
                                .vertices = { { { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                                              { { 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
                                              { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
                                              { { -0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } } },
                                .indices = { 0, 1, 2, 2, 3, 0 } },
                .texture = th::TextureData(std::filesystem::current_path() / "assets" / "grumpy2.jpg"),
                .on_animate = [](th::Model& model) {
                    using namespace std::chrono;
                    static const auto startTime = high_resolution_clock::now();
                    const auto currentTime = high_resolution_clock::now();
                    const auto deltaTime = duration<float, seconds::period>(currentTime - startTime).count();
                    model.transformation.rotate(-deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                } });
    }
};

auto main() -> int {
    auto thyme_api_logger = th::Logger(th::LogLevel::info, "ThymeApi");
    ExampleApp app(thyme_api_logger);
    app.run();
    return 0;
}