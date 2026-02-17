import std;
import glm;
import imgui;

import th.core.logger;
import th.core.application;

import th.scene.model;
import th.scene.camera;
import th.scene.texture_data;

import th.platform.window;
import th.platform.glfw.glfw_window;

import th.gui;

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

class CameraSettings : public th::ui::IComponent {
public:
    explicit CameraSettings(th::Camera& camera) : m_camera(camera) {
        m_camera_position = m_camera.getPosition();
    }
    void draw() override {
        constexpr ImGuiWindowFlags window_flags{};
        ImGui::Begin("viewport settings", nullptr, window_flags);

        static float fov{ 0.0f };
        ImGui::InputFloat("FOV", &fov, 0.01f, 0.1f, "%.2f");

        if (ImGui::InputFloat3("position", glm::value_ptr(m_camera_position))) {
            m_camera.setPosition(m_camera_position);
        }

        ImGui::End();
    }

private:
    th::Camera& m_camera;
    glm::vec3 m_camera_position{ 0.0f, 0.0f, 0.0f };
};

auto main() -> int {
    auto thyme_api_logger = th::Logger(th::LogLevel::info, "ThymeApi");
    ExampleApp app(thyme_api_logger);
    auto camera =
                th::Camera{ th::CameraArguments{ .fov = 45.0f,
                                         .znear = 0.1f,
                                         .zfar = 100.0f,
                                         .resolution = {1280, 720},
                                         .position = { 2.0f, 2.0f, 2.0f },
                                         .center = { 0.0f, 0.0f, 0.0f },
                                         .up = { 0.0f, 0.0f, 1.0f },
                                         .yaw_pitch_roll = th::YawPitchRoll{ .yaw = 0.0f, .pitch = 0.0f, .roll = 0.0f } } };
    auto camera_settings = CameraSettings(camera);
    app.run(camera_settings, camera);
    return 0;
}