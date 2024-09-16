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

public:
    GlfwWindow(const WindowConfiguration& config) : Window{ config } {
        TH_API_LOG_DEBUG("Create window with parameters: width = {}, height = {}, name = {}",
                         config.width,
                         config.height,
                         config.name);
        m_window =
                WindowHWND(glfwCreateWindow(config.width, config.height, config.name.data(), nullptr, nullptr),
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

    virtual void poolEvents() override {
        glfwPollEvents();
    }

    [[nodiscard]] virtual bool shouldClose() override {
        return glfwWindowShouldClose(m_window.get());
    }

private:
    WindowHWND m_window;

    friend Context;
};

class THYME_API VulkanGlfwWindow: public GlfwWindow<VulkanGlfwWindow> {
public:
    VulkanGlfwWindow(const WindowConfiguration& config) : GlfwWindow<VulkanGlfwWindow>(config) {}

    [[nodiscard]] std::vector<std::string> getRequiredInstanceExtensions() const noexcept;

    [[nodiscard]] inline auto getSurface(const vk::UniqueInstance& instance) const {
        VkSurfaceKHR surface{ nullptr };
        if (auto result = glfwCreateWindowSurface(*instance, this->m_window.get(), nullptr, &surface);
            result != VK_SUCCESS) {
            TH_API_LOG_CRITICA("GLFW cannot create VkSurface! Error: {}", static_cast<uint32_t>(result));
            throw std::runtime_error("GLFW cannot create VkSurface!");
        }
        return vk::UniqueSurfaceKHR{ surface, *instance };
    }
};

};// namespace Thyme