#pragma once

#include "thyme/core/logger.hpp"
#include "thyme/core/window.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <memory>

namespace Thyme {

template<typename Context = void>
class GlfwWindow : public Window {
    using WindowHWND = std::unique_ptr<GLFWwindow, std::function<void(GLFWwindow*)>>;

public:
    GlfwWindow(const WindowConfiguration& config);
    virtual void poolEvents() override {
        glfwPollEvents();
    }
    virtual bool shouldClose() override {
        return glfwWindowShouldClose(m_window.get());
    }

    ~GlfwWindow() {
        glfwTerminate();
    }

private:
    WindowHWND m_window;

    friend Context;
};

class THYME_API VulkanGlfwWindow : public GlfwWindow<VulkanGlfwWindow> {
public:
    VulkanGlfwWindow(const WindowConfiguration& config);

    [[nodiscard]] std::vector<std::string> getRequiredInstanceExtensions() const noexcept;

    [[nodiscard]] inline auto getSurface(vk::Instance instance) const noexcept -> VkSurfaceKHR {
        VkSurfaceKHR surface;
        glfwCreateWindowSurface(instance, this->m_window.get(), nullptr, &surface);
        return surface;
    }
};

template<typename Context>
GlfwWindow<Context>::GlfwWindow(const WindowConfiguration& config) : Thyme::Window{ config } {
    TH_API_LOG_DEBUG("Create window with parameters: width = {}, height = {}, name = {}",
                     config.width,
                     config.height,
                     config.name);
    if (glfwInit() == GLFW_FALSE) {
        auto message = "Failed to initialize GLFW!";
        TH_API_LOG_ERROR(message);
        glfwTerminate();
        throw std::runtime_error(message);
    }
    m_window = WindowHWND(glfwCreateWindow(config.width, config.height, config.name.c_str(), nullptr, nullptr),
                          [config](GLFWwindow* window) {
                              TH_API_LOG_DEBUG("destroying window with parameters: width = {}, height = {}, name = {}",
                                               config.width,
                                               config.height,
                                               config.name)
                              if (window != nullptr) {
                                  glfwDestroyWindow(window);
                              }
                          });
};

};// namespace Thyme