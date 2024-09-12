#include "thyme/platform/glfw_window.hpp"

Thyme::VulkanGlfwWindow::VulkanGlfwWindow(const WindowConfiguration& config) : GlfwWindow<VulkanGlfwWindow>{ config } {
    if (glfwVulkanSupported() == GLFW_FALSE) {
        auto message = "GLFW3 does not support vulkan!";
        TH_API_LOG_ERROR(message);
        glfwTerminate();
        throw std::runtime_error(message);
    }
}

std::vector<std::string> Thyme::VulkanGlfwWindow::getRequiredInstanceExtensions() const noexcept {
    uint32_t instanceExtensionCount{ 0 };
    auto instanceExtensionBuffer = glfwGetRequiredInstanceExtensions(&instanceExtensionCount);
    std::vector<std::string> instanceExtension;
    instanceExtension.reserve(instanceExtensionCount);
    for (uint32_t i{ 0 }; i < instanceExtensionCount; ++i) {
        instanceExtension.emplace_back(instanceExtensionBuffer[i]);
    }
    return instanceExtension;
}
