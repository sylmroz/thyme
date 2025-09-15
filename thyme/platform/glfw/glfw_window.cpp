module;

#include <GLFW/glfw3.h>

module th.platform.glfw.glfw_window;

namespace th {

GlfwWindow::GlfwWindow(const WindowConfig& config, Logger& logger) : Window{ config, logger } {
    logger.debug("Create window with parameters: width = {}, height = {}, name = {}",
                 config.width,
                 config.height,
                 config.name);
    initializeContext();
    glfwWindowHint(GLFW_MAXIMIZED, config.maximalized);
    glfwWindowHint(GLFW_DECORATED, config.decorate);
    m_window = WindowHWND(glfwCreateWindow(static_cast<int>(config.width),
                                           static_cast<int>(config.height),
                                           config.name.data(),
                                           nullptr,
                                           nullptr),
                          [name = config.name, &logger](GLFWwindow* window) {
                              logger.debug("Destroying window {}", name);
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
                app->m_eventListener.next(WindowMinimalize{});
            } else {
                app->m_eventListener.next(WindowMaximalize{});
            }
        }
        if (width > 0 && height > 0) {
            app->m_eventListener.next(WindowResize{ .width = width, .height = height });
        }
    });

    glfwSetWindowCloseCallback(m_window.get(), [](GLFWwindow* window) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->m_eventListener.next(WindowClose{});
    });

    glfwSetCursorPosCallback(m_window.get(), [](GLFWwindow* window, double x, double y) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->m_eventListener.next(MousePosition{ { x, y } });
    });

    glfwSetScrollCallback(m_window.get(), [](GLFWwindow* window, double x, double y) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->m_eventListener.next(MouseWheel{ { x, y } });
    });

    glfwSetKeyCallback(
            m_window.get(),
            [](GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods) {
                const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
                const auto& eventSubject = app->m_eventListener;
                if (action == GLFW_PRESS) {
                    eventSubject.next(KeyPressed(static_cast<KeyCode>(key)));
                } else if (action == GLFW_RELEASE) {
                    eventSubject.next(KeyReleased(static_cast<KeyCode>(key)));
                } else if (action == GLFW_REPEAT) {
                    eventSubject.next(KeyRepeated(static_cast<KeyCode>(key)));
                }
            });

    glfwSetMouseButtonCallback(m_window.get(),
                               [](GLFWwindow* window, int button, int action, [[maybe_unused]] int mods) {
                                   const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
                                   const auto& eventSubject = app->m_eventListener;
                                   if (action == GLFW_PRESS) {
                                       eventSubject.next(MouseButtonPress(static_cast<MouseButton>(button)));
                                   } else if (action == GLFW_RELEASE) {
                                       eventSubject.next(MouseButtonReleased(static_cast<MouseButton>(button)));
                                   }
                               });

    glfwSetWindowMaximizeCallback(m_window.get(), [](GLFWwindow* window, int maximize) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        if (maximize == GLFW_TRUE) {
            app->m_eventListener.next(WindowMaximalize{});
        } else {
            app->m_eventListener.next(WindowMinimalize{});
        }
    });
}

}// namespace th
