#pragma once

#include "thyme/core/logger.hpp"
#include "thyme/core/window.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <memory>

namespace Thyme {

template<typename Context = void>
class GlfwWindow: public Window {
    using WindowHWND = std::unique_ptr<GLFWwindow, std::function<void(GLFWwindow*)>>;
    explicit GlfwWindow(const WindowConfiguration& config);

public:
    void poolEvents() override {
        glfwPollEvents();
    }

    [[nodiscard]] bool shouldClose() override {
        return glfwWindowShouldClose(m_window.get()) != 0;
    }

private:
    WindowHWND m_window;

    friend Context;
};
template<typename Context>
GlfwWindow<Context>::GlfwWindow(const WindowConfiguration& config) : Window{ config } {
    TH_API_LOG_DEBUG("Create window with parameters: width = {}, height = {}, name = {}",
                     config.width,
                     config.height,
                     config.name);
    m_window = WindowHWND(glfwCreateWindow(config.width, config.height, config.name.data(), nullptr, nullptr),
                          [config](GLFWwindow* window) {
                              TH_API_LOG_DEBUG("Destroying window with parameters: width = {}, height = {}, name = {}",
                                               config.width,
                                               config.height,
                                               config.name)
                              if (window != nullptr) {
                                  glfwDestroyWindow(window);
                              }
                          });
}

class THYME_API VulkanGlfwWindow final: public GlfwWindow<VulkanGlfwWindow> {
public:
    explicit VulkanGlfwWindow(const WindowConfiguration& config) : GlfwWindow(config) {}

    [[nodiscard]] static std::vector<std::string> getRequiredInstanceExtensions() noexcept;

    [[nodiscard]] auto getSurface(const vk::UniqueInstance& instance) const {
        VkSurfaceKHR surface{ nullptr };
        if (const auto result = glfwCreateWindowSurface(*instance, this->m_window.get(), nullptr, &surface);
            result != VK_SUCCESS) {
            TH_API_LOG_CRITICAL("GLFW cannot create VkSurface! Error: {}", static_cast<uint32_t>(result));
            throw std::runtime_error("GLFW cannot create VkSurface!");
        }
        return vk::UniqueSurfaceKHR{ surface, *instance };
    }
};

};// namespace Thyme