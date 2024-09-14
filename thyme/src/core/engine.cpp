#include "thyme/core/engine.hpp"

#include "thyme/core/logger.hpp"
#include "thyme/platform/glfw_window.hpp"
#include "thyme/platform/vulkan_renderer.hpp"
#include "thyme/version.hpp"

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <memory>
#include <vector>
#include <type_traits>

Thyme::Engine::Engine(const EngineConfig& engineConfig, const PlatformContext& context)
    : m_engineConfig{ engineConfig }, m_context{ context } {}

void Thyme::Engine::run() {
    TH_API_LOG_INFO("Start {} engine", m_engineConfig.engineName);

    VulkanGlfwWindow window(WindowConfiguration{ .width = 1280, .height = 920, .name = m_engineConfig.appName });

    std::vector<const char*> instanceLayers;
    instanceLayers.emplace_back("VK_LAYER_KHRONOS_validation");
    std::vector<const char*> instanceExtension;
    instanceExtension.emplace_back(vk::EXTDebugReportExtensionName);

    auto glfwInstanceExtensions = window.getRequiredInstanceExtensions();
    for (const auto& glfwInstanceExtension : glfwInstanceExtensions) {
        instanceExtension.emplace_back(glfwInstanceExtension.c_str());
    }
    std::vector<const char*> deviceExtension;
    deviceExtension.emplace_back(vk::KHRSwapchainExtensionName);

    auto instance = Vulkan::UniqueInstance(Vulkan::UniqueInstanceConfig{ .engineName = m_engineConfig.engineName,
                                                                         .appName = m_engineConfig.appName,
                                                                         .instanceLayers = instanceLayers,
                                                                         .instanceExtension = instanceExtension });
    const auto surface = window.getSurface(instance.instance);
    const auto physicalDevices = instance.instance.get().enumeratePhysicalDevices();
    std::vector<vk::PhysicalDevice> pickedDevices;
    for (const auto& physicalDevice : physicalDevices) {
        if (Vulkan::QueueFamilyIndices{ physicalDevice, *surface }.isCompleted()) {
            pickedDevices.emplace_back(physicalDevice);
        }
    }

    while (!window.shouldClose()) {
        window.poolEvents();
    }
}
