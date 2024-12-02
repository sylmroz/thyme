module;

#include "thyme/core/logger.hpp"
#include "thyme/pch.hpp"
#include "thyme/platform/vulkan_device_manager.hpp"

#include <ranges>
#include <vector>

#include <imgui.h>


module thyme.core.engine;

import thyme.core.event;
import thyme.core.layer;
import thyme.core.layer_stack;
import thyme.core.utils;
import thyme.core.window;
import thyme.platform.glfw_window;
import thyme.platform.vulkan;

Thyme::Engine::Engine(const EngineConfig& engineConfig, LayerStack<Layer>& layers)
    : m_engineConfig{ engineConfig }, m_layers{ layers } {}

void Thyme::Engine::run() const {
    TH_API_LOG_INFO("Start {} engine", m_engineConfig.engineName);

    EventSubject windowEvents;
    windowEvents.subscribe([&layers = m_layers](const Event& event) {
        for (const auto layer: layers) {
            layer->onEvent(event);
        }
    });
    VulkanGlfwWindow window(WindowConfig{ m_engineConfig, windowEvents });

    const auto glfwExtensions = VulkanGlfwWindow::getRequiredInstanceExtensions();
    // clang-format off
    const std::vector<const char*> enabledExtensions = glfwExtensions
            | std::views::transform([](auto const& layerName) { return layerName.c_str(); })
            | std::ranges::to<std::vector<const char*>>();
    // clang-format on

    const auto instance =
            Vulkan::UniqueInstance(Vulkan::UniqueInstanceConfig{ .engineName = m_engineConfig.engineName,
                                                                 .appName = m_engineConfig.appName,
                                                                 .instanceExtension = enabledExtensions });
    const auto surface = window.getSurface(instance.instance);

    const auto devices = Vulkan::getPhysicalDevices(instance.instance, surface);
    const Vulkan::PhysicalDevicesManager physicalDevicesManager(devices);

    const auto& device = physicalDevicesManager.getSelectedDevice();

    Vulkan::VulkanRenderer renderer(window, device, surface);

    while (!window.shouldClose()) {
        window.poolEvents();
        for (const auto& layer : m_layers) {
            layer->draw();
        }
        renderer.draw();
    }

    device.logicalDevice->waitIdle();
}
