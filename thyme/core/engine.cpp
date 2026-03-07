module;

module th.core.engine;

import std;

import glm;

namespace th {

Engine::Engine(const EngineConfig& engine_config,
               GlfwWindow& window,
               ModelStorage& model_storage,
               WindowEventsHandlers& window_event_handler,
               Logger& logger)
    : m_engine_config{ engine_config }, m_window{ window }, m_window_event_handler{ window_event_handler },
      m_model_storage{ model_storage }, m_logger{ logger } {}

void Engine::run(Camera& camera, ui::IComponent& ui_component) {
    m_logger.info("Start {} engine", m_engine_config.engine_name);

    const auto framework = VulkanFramework::create<GlfwWindow>(
            VulkanFramework::InitInfo{
                    .app_name = m_engine_config.app_name,
                    .engine_name = m_engine_config.engine_name,
            },
            m_logger);

    const auto surface = m_window.createSurface(framework.getInstance());

    const auto physical_devices_manager =
            VulkanPhysicalDevicesManager(framework.getPhysicalDevices(), *surface, m_logger);

    auto& device = physical_devices_manager.getCurrentDevice();
    const auto swapchain_support_details = SwapChainSupportDetails(device.physical_device, *surface);
    const auto graphic_context =
            VulkanGraphicContext{ .max_frames_in_flight = 3,
                                  .image_count = swapchain_support_details.getImageCount(),
                                  .depth_format = findDepthFormat(device.physical_device),
                                  .color_format = vk::Format::eR16G16B16A16Sfloat,
                                  .surface_format = swapchain_support_details.getBestSurfaceFormat(),
                                  .present_mode = swapchain_support_details.getBestPresetMode(),
                                  .sample_count = std::clamp(vk::SampleCountFlagBits::e4,  vk::SampleCountFlagBits::e1, device.max_msaa_samples)};

    Gui gui(device, m_window, graphic_context, *framework.getInstance(), ui_component, m_logger);
    auto buffers_pool = VulkanCommandBuffersPool(device.logical_device,
                                                 device.command_pool,
                                                 device.getGraphicQueue(),
                                                 graphic_context.max_frames_in_flight,
                                                 m_logger);
    const auto frame_buffer_size = m_window.getFrameBufferSize();
    VulkanSwapchain swapchain(device,
                              *surface,
                              graphic_context,
                              swapchain_support_details.getSwapExtent(frame_buffer_size),
                              buffers_pool,
                              m_logger);
    VulkanRenderer renderer(device, swapchain, m_model_storage, camera, gui, graphic_context, buffers_pool, m_logger);

    m_window_event_handler.addEventListener<WindowResizedEvent>([&swapchain](const WindowResizedEvent& window_resized) {
        const auto [width, height] = window_resized;
        swapchain.frameResized(vk::Extent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) });
    });

    while (!m_window.shouldClose()) {
        m_window.poolEvents();
        if (!m_window.isMinimalized()) {
            renderer.draw(device);
        }
    }

    device.logical_device.waitIdle();
}

}// namespace th
