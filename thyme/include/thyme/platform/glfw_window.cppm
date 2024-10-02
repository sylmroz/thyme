module;

#include "thyme/core/logger.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <fmt/format.h>

#include <functional>
#include <memory>

export module thyme.platform.glfw_window;

import thyme.core.event;
import thyme.core.window;

export namespace Thyme {

class GlfwWindow: public Window {
    using WindowHWND = std::unique_ptr<GLFWwindow, std::function<void(GLFWwindow*)>>;
public:
    explicit GlfwWindow(const WindowConfig& config);

    void poolEvents() override {
        glfwPollEvents();
    }

    [[nodiscard]] bool shouldClose() override {
        return glfwWindowShouldClose(m_window.get()) != 0;
    }

    [[nodiscard]] auto& getHandler() const {
        return m_window;
    }

private:
    WindowHWND m_window;
};

class THYME_API VulkanGlfwWindow final: public GlfwWindow {
public:
    explicit VulkanGlfwWindow(const WindowConfig& config) : GlfwWindow(config) {}

    [[nodiscard]] static std::vector<std::string> getRequiredInstanceExtensions() noexcept;

    [[nodiscard]] auto getSurface(const vk::UniqueInstance& instance) const {
        VkSurfaceKHR surface{ nullptr };
        if (const auto result = glfwCreateWindowSurface(*instance, this->getHandler().get(), nullptr, &surface);
            result != VK_SUCCESS) {
            throw std::runtime_error(
                    fmt::format("GLFW cannot create VkSurface! Error: {}", static_cast<uint32_t>(result)));
        }
        return vk::UniqueSurfaceKHR{ surface, *instance };
    }
};

};// namespace Thyme