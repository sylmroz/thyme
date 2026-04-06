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
    WindowedApplication(const WindowedApplicationInitInfo& windowed_application_init_info, Logger& logger);
    void run();

    void update(float delta_time);

    [[nodiscard]] constexpr auto getMaxFramesInFlight() const noexcept -> uint32_t {
        return 2;
    }

private:
    WindowedApplicationInitInfo m_application_init_info;
    WindowEventsHandlers m_window_events_handlers;
    GlfwWindow m_window;
    VulkanFramework m_vulkan_framework;
    vk::raii::SurfaceKHR m_surface;
    PhysicalDevices2 m_physical_devices;
    uint32_t m_queue_family_index;
    vk::raii::Device m_logical_device;
    vk::raii::CommandPool m_command_pool;
    vk::raii::Queue m_queue;
    VulkanCommandBuffersPool2 m_command_buffers_pool;
    VulkanSwapchain2 m_swapchain;
    std::vector<vk::raii::Semaphore> m_image_available_semaphores;

    Logger& m_logger;
};

}// namespace th
