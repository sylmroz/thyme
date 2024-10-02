module;

#include <GLFW/glfw3.h>

#include "thyme/core/logger.hpp"

module thyme.platform.glfw_window;

using namespace Thyme;

GlfwWindow::GlfwWindow(const WindowConfig& config) : Window{ config } {
    TH_API_LOG_DEBUG("Create window with parameters: width = {}, height = {}, name = {}",
                     config.width,
                     config.height,
                     config.name);
    m_window = WindowHWND(glfwCreateWindow(config.width, config.height, config.name.data(), nullptr, nullptr),
                          [](GLFWwindow* window) {
                              TH_API_LOG_DEBUG("Destroying window with parameters: width = {}, height = {}, name = {}",
                                               config.width,
                                               config.height,
                                               config.name)
                              if (window != nullptr) {
                                  glfwDestroyWindow(window);
                              }
                          });
    glfwSetWindowSizeCallback(m_window.get(), [](GLFWwindow* window, int width, int height) {
        auto windowResize = WindowResize{ .width = width, .height = height };
        TH_API_LOG_INFO(windowResize.toString());
    });
}

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
