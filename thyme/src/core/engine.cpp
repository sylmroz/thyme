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

#include <vector>

#include <imgui.h>

import th.render_system.framework;
import th.render_system.framework_factory;

import th.render_system.vulkan.framework;

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


    const auto initInfo = render_system::Framework::InitInfo{
        .appName = m_engineConfig.appName,
        .engineName = m_engineConfig.engineName,
    };
    const auto framework = render_system::vulkan::Framework::create<decltype(window)>(initInfo);
    const auto surface = window.getSurface(framework.getInstance());

    const auto physicalDevicesManager = vulkan::PhysicalDevicesManager(framework.getInstance(), surface.get());

    const auto& device = physicalDevicesManager.getSelectedDevice();
    const auto swapChainDetails = vulkan::SwapChainSupportDetails(device.physicalDevice, surface.get());
    const auto vulkanGraphicContext =
            vulkan::VulkanGraphicContext{ .maxFramesInFlight = 2,
                                          .imageCount = swapChainDetails.getImageCount(),
                                          .depthFormat = vulkan::findDepthFormat(device.physicalDevice),
                                          .colorFormat = vk::Format::eR16G16B16A16Sfloat,
                                          .surfaceFormat = swapChainDetails.getBestSurfaceFormat(),
                                          .presentMode = swapChainDetails.getBestPresetMode() };


    vulkan::Gui gui(device, window, vulkanGraphicContext, framework.getInstance());
    auto buffersPool = vulkan::VulkanCommandBuffersPool(
            device.logicalDevice, device.commandPool, device.getGraphicQueue(), vulkanGraphicContext.maxFramesInFlight);
    vulkan::VulkanSwapChain swapChain(device,
                                      surface.get(),
                                      vulkanGraphicContext,
                                      swapChainDetails.getSwapExtent(window.getFrameBufferSize()),
                                      &buffersPool);
    vulkan::VulkanRenderer renderer(
            device, swapChain, m_modelStorage, m_camera, gui, vulkanGraphicContext, &buffersPool);

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