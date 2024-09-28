module;

#include "thyme/core/logger.hpp"

#include <vulkan/vulkan.hpp>

#include <ranges>
#include <vector>

module thyme.core.engine;

import thyme.platform.glfw_window;
import  thyme.platform.vulkan_renderer;

Thyme::Engine::Engine(const EngineConfig& engineConfig) : m_engineConfig{ engineConfig } {}

void Thyme::Engine::run() {

    TH_API_LOG_INFO("Start {} engine", m_engineConfig.engineName);

    VulkanGlfwWindow window(WindowConfiguration{ m_engineConfig });

    std::vector<const char*> instanceLayers = { "VK_LAYER_KHRONOS_validation" };

    const auto glfwExtensions = VulkanGlfwWindow::getRequiredInstanceExtensions();
    // clang-format off
    const std::vector<const char*> enabledExtensions = glfwExtensions
            | std::views::transform([](auto const& layerName) { return layerName.c_str(); })
            | std::ranges::to<std::vector<const char*>>();
    // clang-format on

    const auto instance =
            Vulkan::UniqueInstance(Vulkan::UniqueInstanceConfig{ .engineName = m_engineConfig.engineName,
                                                                 .appName = m_engineConfig.appName,
                                                                 .instanceLayers = instanceLayers,
                                                                 .instanceExtension = enabledExtensions });
    const auto surface = window.getSurface(instance.instance);

    const auto devices = Vulkan::getPhysicalDevices(instance.instance, surface);
    const Vulkan::PhysicalDevicesManager physicalDevicesManager(devices);

    const auto device = physicalDevicesManager.getSelectedDevice();
    const auto logicalDevice = device.createLogicalDevice();

    while (!window.shouldClose()) {
        window.poolEvents();
    }
}
