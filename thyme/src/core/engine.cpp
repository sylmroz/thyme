#include "thyme/core/engine.hpp"

#include "thyme/core/logger.hpp"
#include "thyme/platform/glfw_window.hpp"
#include "thyme/version.hpp"

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <memory>
#include <vector>

Thyme::Engine::Engine(const EngineConfig& engineConfig) : m_engineConfig{ engineConfig } {}

void Thyme::Engine::run() {
    TH_API_LOG_INFO("Start {} engine", m_engineConfig.engineName);
    VulkanGlfwWindow window(WindowConfiguration{ .width = 1280, .height = 920, .name = m_engineConfig.appName });

    std::vector<const char*> instanceLayers;
    instanceLayers.emplace_back("VK_LAYER_KHRONOS_validation");
    std::vector<const char*> instanceExtension;
    instanceExtension.emplace_back(vk::EXTDebugReportExtensionName);

    auto glfwInstanceExtension = window.getRequiredInstanceExtensions();
    std::vector<const char*> deviceExtension;
    deviceExtension.emplace_back(vk::KHRSwapchainExtensionName);


    vk::ApplicationInfo applicationInfo(m_engineConfig.appName.c_str(),
                                        vk::makeApiVersion(0, Version::major, Version::minor, Version::patch),
                                        m_engineConfig.engineName.c_str(),
                                        vk::makeApiVersion(0, Version::major, Version::minor, Version::patch),
                                        vk::makeApiVersion(1, 3, 290, 0));

    vk::InstanceCreateInfo instanceCreateInfo(
            vk::InstanceCreateFlags(), &applicationInfo, instanceLayers, instanceExtension);
    try {
        auto instance = vk::createInstanceUnique(instanceCreateInfo);
    } catch (vk::SystemError err) {
        TH_API_LOG_ERROR("Failed to create vulkan instance. Message: {}, Code: {}", err.what(), err.code().value());
        throw std::runtime_error("Failed to create vulkan instance.");
    }

    while (!window.shouldClose()) {
        window.poolEvents();
    }
}
