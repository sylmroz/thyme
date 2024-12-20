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
    glfwSetWindowUserPointer(m_window.get(), this);
    glfwSetWindowSizeCallback(m_window.get(), [](GLFWwindow* window, int width, int height) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->config.eventSubject.next(WindowResize{ .width = width, .height = height });
    });

    glfwSetFramebufferSizeCallback(m_window.get(), [](GLFWwindow* window, int, int) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->frameBufferResized = true;
    });

    glfwSetWindowCloseCallback(m_window.get(), [](GLFWwindow* window) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->config.eventSubject.next(WindowClose{});
    });

    glfwSetCursorPosCallback(m_window.get(), [](GLFWwindow* window, double x, double y) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->config.eventSubject.next(MousePosition{ { x, y } });
    });

    glfwSetScrollCallback(m_window.get(), [](GLFWwindow* window, double x, double y) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->config.eventSubject.next(MouseWheel{ { x, y } });
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
