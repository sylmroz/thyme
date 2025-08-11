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

    glfwSetFramebufferSizeCallback(m_window.get(), [](GLFWwindow* window, const int width, const int height) {
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
        if (width > 0 && height > 0) {
            app->config.eventSubject.next(WindowResize{ .width = width, .height = height });
        }
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

    glfwSetKeyCallback(m_window.get(), [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        const auto& eventSubject = app->config.eventSubject;
        if (action == GLFW_PRESS) {
            eventSubject.next(KeyPressed(static_cast<KeyCode>(key)));
        } else if (action == GLFW_RELEASE) {
            eventSubject.next(KeyReleased(static_cast<KeyCode>(key)));
        } else if (action == GLFW_REPEAT) {
            eventSubject.next(KeyRepeated(static_cast<KeyCode>(key)));
        }
    });

    glfwSetMouseButtonCallback(m_window.get(), [](GLFWwindow* window, int button, int action, int mods) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        const auto& eventSubject = app->config.eventSubject;
        if (action == GLFW_PRESS) {
            eventSubject.next(MouseButtonPress(static_cast<MouseButton>(button)));
        } else if (action == GLFW_RELEASE) {
            eventSubject.next(MouseButtonReleased(static_cast<MouseButton>(button)));
        }
    });
}

auto VulkanGlfwWindow::getExtensions() noexcept -> std::vector<std::string> {
    uint32_t instanceExtensionCount{ 0 };
    auto* instanceExtensionBuffer = glfwGetRequiredInstanceExtensions(&instanceExtensionCount);
    std::vector<std::string> instanceExtensions(instanceExtensionCount);
    std::copy_n(instanceExtensionBuffer, instanceExtensionCount, instanceExtensions.begin());
    return instanceExtensions;
}
