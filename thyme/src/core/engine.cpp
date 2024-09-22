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

Thyme::Engine::Engine(EngineConfig engineConfig) : m_engineConfig{std::move( engineConfig )} {}

void Thyme::Engine::run() {
    TH_API_LOG_INFO("Start {} engine", m_engineConfig.engineName);

    VulkanGlfwWindow window(WindowConfiguration{ .width = 1280, .height = 920, .name = m_engineConfig.appName });

    std::vector<const char*> instanceLayers = { "VK_LAYER_KHRONOS_validation" };
    std::vector<const char*> instanceExtension = { vk::EXTDebugUtilsExtensionName };

    auto glfwInstanceExtensions = Thyme::VulkanGlfwWindow::getRequiredInstanceExtensions();
    for (const auto& glfwInstanceExtension : glfwInstanceExtensions) {
        instanceExtension.emplace_back(glfwInstanceExtension.c_str());
    }

    auto instance = Vulkan::UniqueInstance(Vulkan::UniqueInstanceConfig{ .engineName = m_engineConfig.engineName,
                                                                         .appName = m_engineConfig.appName,
                                                                         .instanceLayers = instanceLayers,
                                                                         .instanceExtension = instanceExtension });
    const auto surface = window.getSurface(instance.instance);

    const auto devices = Vulkan::getPhysicalDevices(instance.instance, surface);
    const Vulkan::PhysicalDevicesManager physicalDevicesManager(devices);

    [[maybe_unused]] const auto device = physicalDevicesManager.getSelectedDevice();
    [[maybe_unused]] const auto deviceType = device.physicalDevice.getProperties().deviceType;
    auto ld1 = device.createLogicalDevice();
    auto ld2 = device.createLogicalDevice();
    while (!window.shouldClose()) {
        window.poolEvents();
    }
}
