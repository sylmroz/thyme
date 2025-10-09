module;

module th.core.engine;

import std;

import glm;

namespace th {

Engine::Engine(const EngineConfig& engine_config, GlfwWindow& window, ModelStorage& model_storage, Logger& logger)
    : m_engine_config{ engine_config }, m_camera{ CameraArguments{ .fov = 45.0f,
                                                                   .znear = 0.1f,
                                                                   .zfar = 100.0f,
                                                                   .resolution = window.getFrameBufferSize(),
                                                                   .eye = { 2.0f, 2.0f, 2.0f },
                                                                   .center = { 0.0f, 0.0f, 0.0f },
                                                                   .up = { 0.0f, 0.0f, 1.0f } } },
      m_window{ window }, m_model_storage{ model_storage }, m_logger{ logger } {}

void Engine::run() {
    m_logger.info("Start {} engine", m_engine_config.engine_name);

    WindowResizedEventHandler window_resized_event_handler;
    window_resized_event_handler.subscribe([&camera = m_camera](const WindowResize& window_resized_event) {
        const auto [width, height] = window_resized_event;
        camera.setResolution(glm::vec2{ static_cast<float>(width), static_cast<float>(height) });
    });

    m_window.subscribe([&window_resized_event_handler](const Event& event) {
        std::visit(
                [&](const auto& window_resized_event) {
                    if constexpr (std::is_same_v<std::remove_cvref_t<decltype(window_resized_event)>, WindowResize>) {
                        window_resized_event_handler.next(window_resized_event);
                    }
                },
                event);
    });

    const auto framework = VulkanFramework::create<GlfwWindow>(
            VulkanFramework::InitInfo{
                    .app_name = m_engine_config.app_name,
                    .engine_name = m_engine_config.engine_name,
            },
            m_logger);

    const auto surface = m_window.createSurface(framework.getInstance());

    const auto physical_devices_manager = VulkanPhysicalDevicesManager(framework.getInstance(), *surface, m_logger);

    const auto& device = physical_devices_manager.getSelectedDevice();
    const auto swapchain_support_details = SwapChainSupportDetails(device.physical_device, *surface);
    const auto graphic_context =
            VulkanGraphicContext{ .max_frames_in_flight = 2,
                                  .image_count = swapchain_support_details.getImageCount(),
                                  .depth_format = findDepthFormat(device.physical_device),
                                  .color_format = vk::Format::eR16G16B16A16Sfloat,
                                  .surface_format = swapchain_support_details.getBestSurfaceFormat(),
                                  .present_mode = swapchain_support_details.getBestPresetMode() };


    Gui gui(device, m_window, graphic_context, *framework.getInstance(), m_logger);
    auto buffers_pool = VulkanCommandBuffersPool(device.logical_device,
                                                 device.command_pool,
                                                 device.getGraphicQueue(),
                                                 graphic_context.max_frames_in_flight,
                                                 m_logger);
    const auto frame_buffer_size = m_window.getFrameBufferSize();
    m_camera.setResolution(
            glm::vec2{ static_cast<float>(frame_buffer_size.x), static_cast<float>(frame_buffer_size.y) });
    VulkanSwapchain swapChain(device,
                              *surface,
                              graphic_context,
                              swapchain_support_details.getSwapExtent(frame_buffer_size),
                              buffers_pool,
                              m_logger);
    VulkanRenderer renderer(device, swapChain, m_model_storage, m_camera, gui, graphic_context, buffers_pool);

    window_resized_event_handler.subscribe([&swapChain](const WindowResize& window_resized) {
        const auto [width, height] = window_resized;
        swapChain.frameResized(vk::Extent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) });
    });

    while (!m_window.shouldClose()) {
        m_window.poolEvents();
        if (!m_window.isMinimalized()) {
            renderer.draw();
        }
    }

    device.logical_device.waitIdle();
}

}// namespace th
