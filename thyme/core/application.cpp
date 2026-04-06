module;

module th.core.application;

import glm;
import nlohmann.json;
import vulkan;

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
                VulkanFramework::InitInfo{ .app_name = "Thyme app", .engine_name = "Vulkan backend" }, m_logger);

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

WindowedApplication::WindowedApplication(const WindowedApplicationInitInfo& windowed_application_init_info,
                                         Logger& logger)
    : m_application_init_info(windowed_application_init_info),
      m_window(windowed_application_init_info.window_config, m_window_events_handlers, logger),
      m_vulkan_framework(VulkanFramework::create<GlfwWindow>(
              VulkanFramework::InitInfo{ .app_name = windowed_application_init_info.window_config.name,
                                         .engine_name = windowed_application_init_info.window_config.name },
              logger)),
      m_surface(m_window.createSurface(m_vulkan_framework.getInstance())),
      m_physical_devices(filterDevices(m_vulkan_framework.getPhysicalDevices(), m_surface)),
      m_queue_family_index(selectQueueFamilyIndex(*m_physical_devices.physical_devices.front().physical_device,
                                                  vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer
                                                          | vk::QueueFlagBits::eCompute,
                                                  *m_surface)),
      m_command_pool(m_logical_device.createCommandPool(
              vk::CommandPoolCreateInfo{ .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                         .queueFamilyIndex = m_queue_family_index })),
      m_logical_device(
              createLogicalDevice(m_physical_devices.physical_devices.front().physical_device, m_queue_family_index)),
      m_queue(m_logical_device.getQueue(m_queue_family_index, 0)),
      m_command_buffers_pool(m_logical_device, m_command_pool, m_queue, getMaxFramesInFlight(), logger),
      m_swapchain(
              m_physical_devices.physical_devices.front().physical_device,
              m_logical_device,
              m_queue_family_index,
              m_surface,
              [window = &m_window] {
                  const auto fbs = window->getFrameBufferSize();
                  return vk::Extent2D{ .width = fbs.x, .height = fbs.y };
              },
              logger),
      m_logger(logger) {
    for (size_t i{ 0 }; i < getMaxFramesInFlight(); ++i) {
        m_image_available_semaphores.emplace_back(m_logical_device.createSemaphore(vk::SemaphoreCreateInfo{}));
    }
}

void WindowedApplication::run() {
    m_logger.info("Start application {}"sv, m_application_init_info.window_config.name);
    auto getDT = [old_time = std::chrono::system_clock::now()]() mutable {
        const auto current_time = std::chrono::system_clock::now();
        const auto dt = std::chrono::duration<float>(current_time - old_time);
        old_time = current_time;
        return dt.count();
    };

    RenderGraph renderGraph;


    renderGraph.addTextureResource("swapchain", &m_swapchain);

    renderGraph.addPass("present", [&](RenderGraphBuilder& builder) {
        builder.write("swapchain",
                      ImageTransition{ .layout = vk::ImageLayout::ePresentSrcKHR,
                                       .pipeline_stage = vk::PipelineStageFlagBits2::eBottomOfPipe });
        return [=](RenderGraphContext&, vk::CommandBuffer) {

        };
    });

    while (!m_window.shouldClose()) {
        m_window.poolEvents();
        if (m_window.isMinimalized()) {
            continue;
        }

        if (!m_swapchain.prepareFrame(m_physical_devices.current(), m_logical_device, m_command_buffers_pool)) {
            m_command_buffers_pool.flush(m_logical_device);
            continue;
        }

        update(getDT());
        renderGraph.compile();
        renderGraph.execute(m_command_buffers_pool.get().getBuffer(m_logical_device));
        m_swapchain.submitFrame(m_command_buffers_pool, m_logical_device);
    }

    m_logical_device.waitIdle();
}

void WindowedApplication::update([[maybe_unused]] float delta_time) {}

}// namespace th
