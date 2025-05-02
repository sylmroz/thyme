#include <thyme/platform/vulkan/vulkan_device.hpp>
#include <thyme/core/event.hpp>
#include <thyme/core/logger.hpp>
#include <thyme/core/window.hpp>
#include <thyme/platform/glfw_window.hpp>
#include <thyme/platform/vulkan/gui.hpp>
#include <thyme/platform/vulkan/renderer.hpp>
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

    EventSubject windowEvents;
    windowEvents.subscribe([&layers = m_layers](const Event& event) {
        for (auto* const layer : layers) {
            layer->onEvent(event);
        }
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

    vulkan::Gui gui(device, window, instance.getInstance());
    vulkan::VulkanRenderer renderer(window, device, surface, m_modelStorage, m_camera, gui);

    while (!window.shouldClose()) {
        window.poolEvents();
        renderer.draw();
    }

    device.logicalDevice.waitIdle();
}

}// namespace th