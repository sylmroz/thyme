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
    explicit ExampleApp(th::Logger& logger) : th::Application{ logger } {
        m_model_storage.addModel(th::Model{
                .name = "Grumpy 1",
                .mesh = th::Mesh{ .vertices = { { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
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
                .mesh = th::Mesh{ .vertices = { { { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
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

class CameraSettings: public th::ui::IComponent {
public:
    explicit CameraSettings(th::FpsCamera& camera) : m_camera(camera) {
    }
    void draw() override {
        constexpr ImGuiWindowFlags window_flags{};
        ImGui::Begin("viewport settings", nullptr, window_flags);


        if (ImGui::InputFloat("FOV", &m_camera.camera_arguments.perspective_camera_arguments.fov, 0.01f, 0.1f, "%.2f")) {
            m_camera.camera_arguments.perspective_camera_arguments.fov = std::clamp(m_camera.camera_arguments.perspective_camera_arguments.fov, 5.0f, 120.0f);
            m_camera.updateProjectionMatrix();
        }

        if (ImGui::InputFloat3("position", glm::value_ptr(m_camera.camera_arguments.position))) {
            m_camera.updateViewMatrix();
        }

        if (ImGui::InputFloat("Yaw", &m_camera.camera_arguments.yaw_pitch_roll.yaw, 0.1f, 0.1f, "%.2f")) {
            m_camera.updateViewMatrix();
        }

        if (ImGui::InputFloat("Pith", &m_camera.camera_arguments.yaw_pitch_roll.pitch, 0.1f, 0.1f, "%.2f")) {
            m_camera.updateViewMatrix();
        }

        if (ImGui::InputFloat("Roll", &m_camera.camera_arguments.yaw_pitch_roll.roll, 0.1f, 0.1f, "%.2f")) {
            m_camera.updateViewMatrix();
        }

        ImGui::End();
    }

private:
    th::FpsCamera& m_camera;
};

auto main() -> int {
    auto thyme_api_logger = th::Logger(th::LogLevel::info, "ThymeApi");
    ExampleApp app(thyme_api_logger);

    constexpr auto position = glm::vec3{ 2.0f, 2.0f, 2.0f };
    constexpr auto center = glm::vec3{ 0.0f, 0.0f, 0.0f };
    const auto direction = glm::normalize(position - center);

    const auto yaw_pith_roll = th::calculateYawPithRollAngles(direction, glm::identity<glm::mat3>());

    auto camera = th::FpsCamera(th::FpsCameraArguments{
            .perspective_camera_arguments =
                    th::PerspectiveCameraArguments{
                            .fov = 45.0f, .znear = 0.1f, .zfar = 100.0f, .resolution = { 1280, 720 } },
            .position = { 2.0f, 2.0f, 2.0f },
            .direction = { 0.0f, 1.0f, 0.0f },
            .up = { 0.0f, 0.0f, 1.0f },
            .yaw_pitch_roll = th::YawPitchRoll{ .yaw = 0.0f, .pitch = 0.0f, .roll = 0.0f } });
    auto camera_settings = CameraSettings(camera);
    app.run(camera_settings, camera);
    return 0;
}