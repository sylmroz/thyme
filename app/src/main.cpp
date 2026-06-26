import std;
import glm;
import imgui;
import vulkan;

import th.core.logger;
import th.core.application;

import th.scene.model;
import th.scene.camera;
import th.scene.texture_data;

import th.platform.window;
import th.platform.glfw.glfw_window;

import th.render_system.render_graph;
import th.render_system.passes;
import th.render_system.vulkan;
import th.render_system.renderer;

import th.gui;

class ThymeApp: public th::WindowedApplication {
public:
    ThymeApp(const th::WindowedApplicationInitInfo& windowed_application_init_info, th::Logger& logger)
        : th::WindowedApplication(windowed_application_init_info, logger),
          m_uniform_buffer(m_renderer.createUniformBuffer<glm::mat4>(m_allocator)),
          m_my_pass(m_physical_devices.current(),
                    m_logical_device,
                    m_swapchain.getFormat(),
                    m_uniform_buffer.getDescriptorBufferInfos(),
                    logger),
          m_camera(th::FpsCameraViewArguments{ .position = glm::vec3(0.0f, 0.0f, 2.0f) },
                   th::PerspectiveCameraArguments{ .fov = 45.0f,
                                                   .znear = 0.1f,
                                                   .zfar = 100.0f,
                                                   .aspect_ratio = 1280.0f / 720.0f }),
          m_camera_controller(std::ref(m_camera), m_window_events_handlers) {};

    void update(float dt, th::RenderGraph& render_graph) override {
        m_camera_controller.update(dt);
        m_camera.setResolution(m_window.getFrameBufferSize());
        m_uniform_buffer.update(m_camera.getViewProjectionMatrix());
        const auto resource = render_graph.addTextureResource("swapchain", m_swapchain);
        m_my_pass.setup(render_graph, resource);
    }
    ~ThymeApp() override = default;

private:
    th::UniformBuffer<glm::mat4> m_uniform_buffer;
    th::MyPass m_my_pass;
    th::FpsCamera m_camera;
    th::CameraController m_camera_controller;
};

auto main() -> int {
    auto thyme_api_logger = th::Logger(th::LogLevel::debug, "ThymeApi");
    auto app = ThymeApp(
            th::WindowedApplicationInitInfo{
                    .window_config = th::WindowConfig{ .width = 1280, .height = 720, .name = "Thyme app" } },
            thyme_api_logger);
    app.run();
    /*ExampleApp app(thyme_api_logger);
    auto camera = th::FpsCamera(th::FpsCameraArguments{
            .perspective_camera_arguments =
                    th::PerspectiveCameraArguments{
                            .fov = 45.0f, .znear = 0.1f, .zfar = 100.0f, .resolution = { 1280, 720 } },
            .position = { 2.0f, 2.0f, 2.0f },
            .direction = glm::normalize(glm::vec3{ -1.0f, -1.0f, -1.0f }),
            .up = { 0.0f, 0.0f, 1.0f },
            .yaw_pitch_roll = th::YawPitchRoll{ .yaw = 135.0f, .pitch = -45.0f, .roll = 0.0f } });
    auto camera_settings = CameraSettings(camera);
    app.run(camera_settings, camera);*/
    return 0;
}