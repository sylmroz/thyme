#include "thyme/platform/glfw_window.hpp"

#include "thyme/core/logger.hpp"

Thyme::GlfwWindow::GlfwWindow(const WindowConfiguration& config)
    : Thyme::Window{ config },
      m_window{ WindowHWND(glfwCreateWindow(config.width, config.height, config.name.c_str(), nullptr, nullptr),
                           [config](GLFWwindow* window) {
                               TH_API_LOG_DEBUG(
                                       "destroying window with parameters: width = {}, height = {}, name = {}",
                                       config.width,
                                       config.height,
                                       config.name)
                               if (window != nullptr) {
                                   glfwDestroyWindow(window);
                               }
                           }) } {
    TH_API_LOG_DEBUG("Create window with parameters: width = {}, height = {}, name = {}",
                     config.width,
                     config.height,
                     config.name);
};

void Thyme::GlfwWindow::poolEvents() {
    glfwPollEvents();
}

bool Thyme::GlfwWindow::shouldClose() {
    return glfwWindowShouldClose(m_window.get());
}
