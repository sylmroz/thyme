export module th.core.application;

import std;
import vulkan;

import th.core.engine;
import th.core.events;
import th.core.logger;
import th.scene.model;
import th.scene.camera;
import th.platform.imgui_context;
import th.platform.window_event_handler;
import th.platform.window;
import th.platform.glfw.glfw_window;
import th.render_system.vulkan;
import th.render_system.renderer;
import th.render_system.render_graph;
import th.gui;

namespace th {

export class Application {
public:
    explicit Application(Logger& logger);

    Application(const Application&) = delete;
    Application(Application&&) = delete;
    auto operator=(const Application&) -> Application& = delete;
    auto operator=(Application&&) -> Application& = delete;
    virtual ~Application() = default;

    void run(ui::IComponent& component, Camera& camera);

protected:
    ModelStorage m_model_storage;

private:
    std::string m_name{ "Thyme" };
    Logger& m_logger;
};

export struct WindowedApplicationInitInfo {
    WindowConfig window_config;
};

export class WindowedApplication {
public:
    virtual ~WindowedApplication() = default;
    WindowedApplication(const WindowedApplicationInitInfo& windowed_application_init_info, Logger& logger);
    void run();

    virtual void update(float dt, RenderGraph& render_graph) = 0;

    [[nodiscard]] constexpr auto getMaxFramesInFlight() const noexcept -> uint32_t {
        return 2;
    }

protected:
    WindowedApplicationInitInfo m_application_init_info;
    WindowEventsHandlers m_window_events_handlers;
    GlfwWindow m_window;
    VulkanFramework m_vulkan_framework;
    vk::raii::SurfaceKHR m_surface;

    PhysicalDevices2 m_physical_devices;
    uint32_t m_queue_family_index;
    vk::raii::Device m_logical_device;

    Renderer m_renderer;

    VulkanSwapchain2 m_swapchain;
    std::vector<vk::raii::Semaphore> m_image_available_semaphores;

    Logger& m_logger;
};

}// namespace th
