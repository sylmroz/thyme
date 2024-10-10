module;

#include "thyme/core/logger.hpp"
#include "thyme/platform/vulkan_device_manager.hpp"


#include <ranges>
#include <vector>

module thyme.core.engine;

import thyme.core.window;
import thyme.platform.glfw_window;
import thyme.platform.vulkan_renderer;

Thyme::Engine::Engine(const EngineConfig& engineConfig) : m_engineConfig{ engineConfig } {}

void Thyme::Engine::run() {

    TH_API_LOG_INFO("Start {} engine", m_engineConfig.engineName);

    VulkanGlfwWindow window(WindowConfig{ m_engineConfig });

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

    const auto device = physicalDevicesManager.getSelectedDevice();
    const auto logicalDevice = device.createLogicalDevice();

    [[maybe_unused]] const auto graphicQueue = logicalDevice->getQueue(device.queueFamilyIndices.graphicFamily.value(), 0);
    [[maybe_unused]] const auto presentationQueue =
            logicalDevice->getQueue(device.queueFamilyIndices.presentFamily.value(), 0);

    const auto& swapChainSupportDetails = device.swapChainSupportDetails;
    const auto surfaceFormat = swapChainSupportDetails.getBestSurfaceFormat();
    const auto presetMode = swapChainSupportDetails.getBestPresetMode();
    const auto extent = swapChainSupportDetails.getSwapExtent(window.getFrameBufferSize());

    while (!window.shouldClose()) {
        window.poolEvents();
    }
}
