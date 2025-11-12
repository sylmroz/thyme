module;

#include <vulkan/vulkan.h>

module th.platform.glfw.glfw_window;

import std;

import vulkan_hpp;
import glfw;

namespace th {

GlfwWindow::GlfwWindow(const WindowConfig& config, WindowEventsHandlers& event_handlers, Logger& logger)
    : Window{ config, event_handlers, logger } {
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
        const auto glfw_window = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        if (const auto state = (width == 0 || height == 0) ? WindowState::minimalized : WindowState::maximalized;
            state != glfw_window->m_window_state) {
            glfw_window->m_window_state = state;
            glfw_window->m_event_handlers.onEvent(WindowMinimalizedEvent{ .minimized = state == WindowState::minimalized });
        }
        if (width > 0 && height > 0) {
            glfw_window->m_event_handlers.onEvent(WindowResizedEvent{ .width = width, .height = height });
        }
    });

    glfwSetWindowCloseCallback(m_window.get(), [](GLFWwindow* window) {
        const auto glfw_window = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        glfw_window->m_event_handlers.onEvent(WindowClosedEvent{});
    });

    glfwSetCursorPosCallback(m_window.get(), [](GLFWwindow* window, double x, double y) {
        const auto glfw_window = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        glfw_window->m_event_handlers.onEvent(MousePositionEvent{ { x, y } });
    });

    glfwSetScrollCallback(m_window.get(), [](GLFWwindow* window, double x, double y) {
        const auto glfw_window = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        glfw_window->m_event_handlers.onEvent(MouseWheelEvent{ { x, y } });
    });

    glfwSetKeyCallback(m_window.get(),
                       [](GLFWwindow* window,
                          int key,
                          [[maybe_unused]] int scancode,
                          const int action,
                          [[maybe_unused]] int mods) {
                           const auto glfw_window = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
                           const auto key_code = static_cast<KeyCode>(key);
                           if (action == glfw_press) {
                               glfw_window->m_event_handlers.onEvent(KeyPressedEvent{key_code});
                           } else if (action == glfw_release) {
                               glfw_window->m_event_handlers.onEvent(KeyReleasedEvent{key_code});
                           } else if (action == glfw_repeat) {
                               glfw_window->m_event_handlers.onEvent(KeyRepeatedEvent{key_code});
                           }
                       });

    glfwSetMouseButtonCallback(m_window.get(),
                               [](GLFWwindow* window, int button, const int action, [[maybe_unused]] int mods) {
                                   const auto glfw_window = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
                                   const auto mouse_button = static_cast<MouseButton>(button);
                                   if (action == glfw_press) {
                                       glfw_window->m_event_handlers.onEvent(MouseButtonPressedEvent{mouse_button});
                                   } else if (action == glfw_release) {
                                       glfw_window->m_event_handlers.onEvent(MouseButtonReleasedEvent{mouse_button});
                                   }
                               });

    glfwSetWindowMaximizeCallback(m_window.get(), [](GLFWwindow* window, const int maximize) {
        const auto glfw_window = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        glfw_window->m_event_handlers.onEvent(WindowMaximizedEvent{ .maximized = maximize == glfw_true });
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
