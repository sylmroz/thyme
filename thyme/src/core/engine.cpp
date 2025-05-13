#include <thyme/core/event.hpp>
#include <thyme/core/logger.hpp>
#include <thyme/core/window.hpp>
#include <thyme/platform/glfw_window.hpp>
#include <thyme/platform/vulkan/gui.hpp>
#include <thyme/platform/vulkan/renderer.hpp>
#include <thyme/platform/vulkan/vulkan_command_buffers.hpp>
#include <thyme/platform/vulkan/vulkan_device.hpp>
#include <thyme/platform/vulkan/vulkan_graphic_context.hpp>
#include <thyme/platform/vulkan/vulkan_layer.hpp>

#include <ranges>
#include <vector>

#include <imgui.h>

namespace th {

Engine::Engine(const EngineConfig& engineConfig, vulkan::VulkanLayerStack& layers, scene::ModelStorage& modelStorage)
    : m_engineConfig{ engineConfig },
      m_camera{ scene::CameraArguments{ .fov = 45.0f,
                                        .zNear = 0.1f,
                                        .zFar = 100.0f,
                                        .resolution = { engineConfig.width, engineConfig.height },
                                        .eye = { 2.0f, 2.0f, 2.0f },
                                        .center = { 0.0f, 0.0f, 0.0f },
                                        .up = { 0.0f, 0.0f, 1.0f } } },
      m_layers{ layers }, m_modelStorage{ modelStorage } {}

void Engine::run() {
    TH_API_LOG_INFO("Start {} engine", m_engineConfig.engineName);

    WindowResizedEventHandler windowResizedEventHandler;
    EventSubject windowEvents;
    windowEvents.subscribe([&layers = m_layers, &windowResizedEventHandler](const Event& event) {
        for (auto* const layer : layers) {
            layer->onEvent(event);
        }
        std::visit(
                [&](const auto& windowResizedEvent) {
                    if constexpr (std::is_same_v<std::remove_cvref_t<decltype(windowResizedEvent)>, WindowResize>) {
                        windowResizedEventHandler.next(windowResizedEvent);
                    }
                },
                event);
    });
    windowResizedEventHandler.subscribe([&camera = m_camera](const WindowResize& windowResizedEvent) {
        const auto [width, height] = windowResizedEvent;
        camera.setResolution(glm::vec2{ static_cast<float>(width), static_cast<float>(height) });
    });
    VulkanGlfwWindow window(WindowConfig{ m_engineConfig, windowEvents });

    const auto glfwExtensions = VulkanGlfwWindow::getRequiredInstanceExtensions();
    // clang-format off
    const auto enabledExtensions = glfwExtensions
            | std::views::transform([](auto const& layerName) { return layerName.c_str(); })
            | std::ranges::to<std::vector<const char*>>();
    // clang-format on

    const auto instance =
            vulkan::UniqueInstance(vulkan::UniqueInstanceConfig{ .engineName = m_engineConfig.engineName,
                                                                 .appName = m_engineConfig.appName,
                                                                 .instanceExtension = enabledExtensions });
    const auto surface = window.getSurface(instance.getInstance());

    const auto physicalDevicesManager = vulkan::PhysicalDevicesManager(instance.getInstance(), surface.get());

    const auto& device = physicalDevicesManager.getSelectedDevice();
    const auto swapChainDetails = vulkan::SwapChainSupportDetails(device.physicalDevice, surface.get());
    const auto vulkanGraphicContext =
            vulkan::VulkanGraphicContext{ .maxFramesInFlight = 2,
                                          .imageCount = swapChainDetails.getImageCount(),
                                          .depthFormat = vulkan::findDepthFormat(device.physicalDevice),
                                          .surfaceFormat = swapChainDetails.getBestSurfaceFormat(),
                                          .presentMode = swapChainDetails.getBestPresetMode() };


    vulkan::VulkanCommandBuffersPool buffersPool(
            device.logicalDevice, device.commandPool, device.getGraphicQueue(), vulkanGraphicContext.maxFramesInFlight);
    vulkan::Gui gui(device, window, vulkanGraphicContext, instance.getInstance());
    vulkan::VulkanSwapChain swapChain(
            device, surface.get(), vulkanGraphicContext, swapChainDetails.getSwapExtent(window.getFrameBufferSize()), &buffersPool);
    vulkan::VulkanRenderer renderer(device, swapChain, m_modelStorage, m_camera, gui, vulkanGraphicContext, &buffersPool);

    windowResizedEventHandler.subscribe([&swapChain](const WindowResize& windowResized) {
        const auto [width, height] = windowResized;
        swapChain.frameResized(vk::Extent2D{ static_cast<uint32_t>(width), static_cast<glm::uint32>(height) });
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