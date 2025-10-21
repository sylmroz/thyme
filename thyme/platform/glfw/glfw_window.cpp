module;

#include <vulkan/vulkan.h>

module th.platform.glfw.glfw_window;

import std;

import vulkan_hpp;
import glfw;

namespace th {

GlfwWindow::GlfwWindow(const WindowConfig& config, Logger& logger) : Window{ config, logger } {
    logger.debug("Create window with parameters: width = {}, height = {}, name = {}",
                 config.width,
                 config.height,
                 config.name);
    initializeContext();
    glfwWindowHint(glfw_maximized, config.maximized);
    glfwWindowHint(glfw_decorate, config.decorate);
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
        if (const auto state = (width == 0 || height == 0) ? WindowState::minimalized : WindowState::maximalized;
            state != app->m_window_state) {
            app->m_window_state = state;
            if (state == WindowState::minimalized) {
                app->m_event_listener.next(WindowMinimalize{ .minimized = true });
            } else {
                app->m_event_listener.next(WindowMinimalize{ .minimized = false });
            }
        }
        if (width > 0 && height > 0) {
            app->m_event_listener.next(WindowResize{ .width = width, .height = height });
        }
    });

    glfwSetWindowCloseCallback(m_window.get(), [](GLFWwindow* window) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->m_event_listener.next(WindowClose{});
    });

    glfwSetCursorPosCallback(m_window.get(), [](GLFWwindow* window, double x, double y) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->m_event_listener.next(MousePosition{ { x, y } });
    });

    glfwSetScrollCallback(m_window.get(), [](GLFWwindow* window, double x, double y) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->m_event_listener.next(MouseWheel{ { x, y } });
    });

    glfwSetKeyCallback(m_window.get(),
                       [](GLFWwindow* window,
                          int key,
                          [[maybe_unused]] int scancode,
                          const int action,
                          [[maybe_unused]] int mods) {
                           const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
                           const auto& eventSubject = app->m_event_listener;
                           if (action == glfw_press) {
                               eventSubject.next(KeyPressed(static_cast<KeyCode>(key)));
                           } else if (action == glfw_release) {
                               eventSubject.next(KeyReleased(static_cast<KeyCode>(key)));
                           } else if (action == glfw_repeat) {
                               eventSubject.next(KeyRepeated(static_cast<KeyCode>(key)));
                           }
                       });

    glfwSetMouseButtonCallback(m_window.get(),
                               [](GLFWwindow* window, int button, const int action, [[maybe_unused]] int mods) {
                                   const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
                                   const auto& eventSubject = app->m_event_listener;
                                   if (action == glfw_press) {
                                       eventSubject.next(MouseButtonPress(static_cast<MouseButton>(button)));
                                   } else if (action == glfw_release) {
                                       eventSubject.next(MouseButtonReleased(static_cast<MouseButton>(button)));
                                   }
                               });

    glfwSetWindowMaximizeCallback(m_window.get(), [](GLFWwindow* window, int maximize) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->m_event_listener.next(WindowMaximize{ .maximized = maximize == glfw_true });
    });
}

auto GlfwWindow::createSurface(const vk::raii::Instance& instance) const -> vk::raii::SurfaceKHR {
    VkSurfaceKHR surface{ nullptr };
    if (const auto result = glfwCreateWindowSurface(*instance, this->getHandler().get(), nullptr, &surface);
        result != VK_SUCCESS) {
        throw std::runtime_error(
                std::format("GLFW cannot create VkSurface! Error: {}", vk::to_string(static_cast<vk::Result>(result))));
    }
    return vk::raii::SurfaceKHR(instance, surface);
}

}// namespace th
