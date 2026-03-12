module;

module th.core.application;

import glm;
import nlohmann.json;

import th.core.events;
import th.platform.window;
import th.platform.window_event_handler;
import th.platform.window_settings;
import th.platform.glfw.glfw_window;
import th.render_system.vulkan;

namespace th {

using namespace std::string_view_literals;

Application::Application(Logger& logger) : m_model_storage{ logger }, m_logger{ logger } {}

void Application::run(ui::IComponent& component, Camera& camera) {
    m_logger.info("Starting Thyme api {}"sv, m_name);
    try {
        auto window_settings =
                WindowSettings(WindowConfig{ .width = 1280, .height = 720, .name = "Thyme", .maximized = false });
        auto window_event_handlers = WindowEventsHandlers();
        auto window = GlfwWindow(window_settings.getConfig(), window_event_handlers, m_logger);
        camera.setResolution(window.getFrameBufferSize());


        window_event_handlers.addEventListener<WindowResizedEvent>(
                [&window_settings, &camera](const WindowResizedEvent& window_resize) {
                    const auto [width, height] = window_resize;
                    window_settings.setResolution(glm::uvec2(width, height));
                    camera.setResolution(glm::vec2{ static_cast<float>(width), static_cast<float>(height) });
                });

        window_event_handlers.addEventListener<WindowMaximizedEvent>(
                [&window_settings](const WindowMaximizedEvent maximize) {
                    window_settings.setMaximized(maximize.maximized);
                });

        const auto framework = VulkanFramework::create<GlfwWindow>(
        VulkanFramework::InitInfo{
                .app_name = "Thyme app",
                .engine_name = "Vulkan backend"
        },
        m_logger);

        const auto surface = window.createSurface(framework.getInstance());

        const auto physical_devices_manager =
                VulkanPhysicalDevicesManager(framework.getPhysicalDevices(), *surface, m_logger);

        auto& device = physical_devices_manager.getCurrentDevice();

        auto engine =
                Engine(EngineConfig{ .app_name = m_name }, window, m_model_storage, window_event_handlers, m_logger);
        engine.run(camera, component, surface, device, framework.getInstance());
    } catch (const std::exception& e) {
        m_logger.error("Error occurred during app runtime\n Error: {}"sv, e.what());
    } catch (...) {
        m_logger.error("Unknown error occurred during app runtime"sv);
    }
}

}// namespace th
