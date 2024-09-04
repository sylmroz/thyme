#include "thyme/core/engine.hpp"

#include "thyme/core/logger.hpp"
#include "thyme/platform/glfw_window.hpp"

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <memory>
#include <vector>

void Thyme::Engine::run() {
    TH_API_LOG_INFO("Start Thyme engine");

    glfwInit();
    if (glfwVulkanSupported() == GLFW_FALSE) {
        auto message = "GLFW3 does not support vulkan!";
        TH_API_LOG_ERROR(message);
        glfwTerminate();
        throw std::runtime_error(message);
    }

    std::vector<const char*> instanceLayers;
    instanceLayers.emplace_back("VK_LAYER_KHRONOS_validation");
    std::vector<const char*> instanceExtension;
    instanceExtension.emplace_back(vk::EXTDebugReportExtensionName);
    std::vector<const char*> deviceExtension;
    deviceExtension.emplace_back(vk::KHRSwapchainExtensionName);

    uint32_t instanceExtensionCount{ 0 };
    auto instanceExtensionBuffer = glfwGetRequiredInstanceExtensions(&instanceExtensionCount);
    for (uint32_t i{ 0 }; i < instanceExtensionCount; ++i) {
        instanceExtension.emplace_back(instanceExtensionBuffer[i]);
    }

    vk::ApplicationInfo applicationInfo("Thyme",
                                        vk::makeApiVersion(0, 0, 1, 0),
                                        "Thyme",
                                        vk::makeApiVersion(0, 0, 1, 0),
                                        vk::makeApiVersion(1, 3, 290, 0));

    vk::InstanceCreateInfo instanceCreateInfo(
            vk::InstanceCreateFlags(), &applicationInfo, instanceLayers, instanceExtension);
    try {
        auto instance = vk::createInstanceUnique(instanceCreateInfo);
    } catch (vk::SystemError err) {
        TH_API_LOG_ERROR("Failed to create vulkan instance. Message: {}, Code: {}", err.what(), err.code().value());
        throw std::runtime_error("Failed to create vulkan instance.");
    }
    
    GlfwWindow window(WindowConfiguration{ .width = 1280, .height = 920, .name = "Thyme App" });

    while (!window.shouldClose()) {
        window.poolEvents();
    }
}
