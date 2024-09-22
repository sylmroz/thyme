#include "thyme/core/engine.hpp"

#include "thyme/core/logger.hpp"
#include "thyme/platform/glfw_window.hpp"
#include "thyme/platform/vulkan_renderer.hpp"
#include "thyme/version.hpp"

#include <utility>
#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <memory>
#include <type_traits>
#include <vector>

Thyme::Engine::Engine(EngineConfig engineConfig) : m_engineConfig{ std::move(engineConfig) } {}

void Thyme::Engine::run() {
    TH_API_LOG_INFO("Start {} engine", m_engineConfig.engineName);

    VulkanGlfwWindow window(WindowConfiguration{ .width = 1280, .height = 920, .name = m_engineConfig.appName });

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
