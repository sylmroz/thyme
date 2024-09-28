module;

#include <GLFW/glfw3.h>

module thyme.platform.glfw_window;

std::vector<std::string> Thyme::VulkanGlfwWindow::getRequiredInstanceExtensions() noexcept {

    uint32_t instanceExtensionCount{ 0 };
    auto* instanceExtensionBuffer = glfwGetRequiredInstanceExtensions(&instanceExtensionCount);
    std::vector<std::string> instanceExtension;
    instanceExtension.reserve(instanceExtensionCount);
    for (uint32_t i{ 0 }; i < instanceExtensionCount; ++i) {
        instanceExtension.emplace_back(instanceExtensionBuffer[i]);
    }
    return instanceExtension;
}
