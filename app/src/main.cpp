#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
//#include <thyme/core/core.hpp>

import th.core.logger;
import th.core.application;

import th.scene.model;
import th.scene.texture_data;

class ExampleApp: public th::core::Application {
public:
    ExampleApp() {
        modelStorage.addModel(th::scene::Model{
                .name = "Grumpy 1",
                .mesh =
                        th::scene::Mesh{
                                .vertices = { { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                                              { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
                                              { { 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
                                              { { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } } },
                                .indices = { 0, 1, 2, 2, 3, 0 } },
                .texture = th::scene::TextureData("C:\\Users\\sylwek\\Desktop\\grumpy.jpg"),
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
                .texture = th::scene::TextureData("C:\\Users\\sylwek\\Desktop\\grumpy2.jpg"),
                .onAnimate = [](th::scene::Model& model) {
                    using namespace std::chrono;
                    static const auto startTime = high_resolution_clock::now();
                    const auto currentTime = high_resolution_clock::now();
                    const auto deltaTime = duration<float, seconds::period>(currentTime - startTime).count();
                    model.transformation.rotate(-deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                } });
    }
};

int main() {
    th::core::AppLogger::init(th::core::LogLevel::info);
    th::core::AppLogger::getLogger()->info("Hello from app");
    ExampleApp app;
    app.name = "AppThyme";
    app.run();

    return 0;
}