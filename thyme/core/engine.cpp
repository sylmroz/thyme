module;

module th.core.engine;

namespace th {

Engine::Engine(const EngineConfig& engineConfig, ModelStorage& modelStorage, Logger& logger)
    : m_engineConfig{ engineConfig },
      m_camera{ CameraArguments{ .fov = 45.0f,
                                 .zNear = 0.1f,
                                 .zFar = 100.0f,
                                 .resolution = { engineConfig.width, engineConfig.height },
                                 .eye = { 2.0f, 2.0f, 2.0f },
                                 .center = { 0.0f, 0.0f, 0.0f },
                                 .up = { 0.0f, 0.0f, 1.0f } } },
      m_modelStorage{ modelStorage }, m_logger{ logger } {}

void Engine::run() {
    m_logger.info("Start {} engine", m_engineConfig.engineName);

    WindowResizedEventHandler windowResizedEventHandler;
    windowResizedEventHandler.subscribe([&camera = m_camera](const WindowResize& windowResizedEvent) {
        const auto [width, height] = windowResizedEvent;
        camera.setResolution(glm::vec2{ static_cast<float>(width), static_cast<float>(height) });
    });

    auto window = VulkanGlfwWindow(WindowConfig{ .width = m_engineConfig.width,
                                                 .height = m_engineConfig.height,
                                                 .name = "Thyme",
                                                 .maximalized = true },
                                   m_logger);
    window.subscribe([&windowResizedEventHandler](const Event& event) {
        std::visit(
                [&](const auto& windowResizedEvent) {
                    if constexpr (std::is_same_v<std::remove_cvref_t<decltype(windowResizedEvent)>, WindowResize>) {
                        windowResizedEventHandler.next(windowResizedEvent);
                    }
                },
                event);
    });

    const auto framework = VulkanFramework::create<VulkanGlfwWindow>(
            VulkanFramework::InitInfo{
                    .appName = m_engineConfig.appName,
                    .engineName = m_engineConfig.engineName,
            },
            m_logger);

    const auto surface = window.createSurface(framework.getInstance());

    const auto physicalDevicesManager = VulkanPhysicalDevicesManager(framework.getInstance(), *surface, m_logger);

    const auto& device = physicalDevicesManager.getSelectedDevice();
    const auto swapChainDetails = SwapChainSupportDetails(device.physicalDevice, *surface);
    const auto graphicContext = VulkanGraphicContext{ .maxFramesInFlight = 2,
                                                      .imageCount = swapChainDetails.getImageCount(),
                                                      .depthFormat = findDepthFormat(device.physicalDevice),
                                                      .colorFormat = vk::Format::eR16G16B16A16Sfloat,
                                                      .surfaceFormat = swapChainDetails.getBestSurfaceFormat(),
                                                      .presentMode = swapChainDetails.getBestPresetMode() };


    Gui gui(device, window, graphicContext, *framework.getInstance(), m_logger);
    auto buffersPool = VulkanCommandBuffersPool(device.logicalDevice,
                                                device.commandPool,
                                                device.getGraphicQueue(),
                                                graphicContext.maxFramesInFlight,
                                                m_logger);
    const auto frame_buffer_size = window.getFrameBufferSize();
    m_camera.setResolution(
            glm::vec2{ static_cast<float>(frame_buffer_size.x), static_cast<float>(frame_buffer_size.y) });
    VulkanSwapChain swapChain(device,
                              *surface,
                              graphicContext,
                              swapChainDetails.getSwapExtent(window.getFrameBufferSize()),
                              buffersPool,
                              m_logger);
    VulkanRenderer renderer(device, swapChain, m_modelStorage, m_camera, gui, graphicContext, buffersPool);

    windowResizedEventHandler.subscribe([&swapChain](const WindowResize& windowResized) {
        const auto [width, height] = windowResized;
        swapChain.frameResized(vk::Extent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) });
    });

    while (!window.shouldClose()) {
        window.poolEvents();
        if (!window.isMinimalized()) {
            renderer.draw();
        }
    }

    device.logicalDevice.waitIdle();
}

}// namespace th
