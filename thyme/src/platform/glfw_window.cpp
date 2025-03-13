#include <thyme/core/event.hpp>
#include <thyme/core/logger.hpp>
#include <thyme/platform/glfw_window.hpp>

#include <GLFW/glfw3.h>

using namespace th;

GlfwWindow::GlfwWindow(const WindowConfig& config) : Window{ config } {
    TH_API_LOG_DEBUG("Create window with parameters: width = {}, height = {}, name = {}",
                     config.width,
                     config.height,
                     config.name);
    m_window = WindowHWND(glfwCreateWindow(static_cast<int>(config.width),
                                           static_cast<int>(config.height),
                                           config.name.data(),
                                           nullptr,
                                           nullptr),
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
    glfwSetWindowSizeCallback(m_window.get(), [](GLFWwindow* window, const int width, const int height) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->config.eventSubject.next(WindowResize{ .width = width, .height = height });
    });

    glfwSetFramebufferSizeCallback(m_window.get(), [](GLFWwindow* window, int width, int height) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        const auto state = (width == 0 || height == 0) ? WindowState::minimalized : WindowState::maximalized;
        if (state != app->m_windowState) {
            app->m_windowState = state;
            if (state == WindowState::minimalized) {
                app->config.eventSubject.next(WindowMinimalize{});
            } else {
                app->config.eventSubject.next(WindowMaximalize{});
            }
        }
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

std::vector<std::string> th::VulkanGlfwWindow::getRequiredInstanceExtensions() noexcept {
    uint32_t instanceExtensionCount{ 0 };
    auto* instanceExtensionBuffer = glfwGetRequiredInstanceExtensions(&instanceExtensionCount);
    std::vector<std::string> instanceExtension;
    instanceExtension.reserve(instanceExtensionCount);
    for (uint32_t i{ 0 }; i < instanceExtensionCount; ++i) {
        instanceExtension.emplace_back(instanceExtensionBuffer[i]);
    }
    return instanceExtension;
}
