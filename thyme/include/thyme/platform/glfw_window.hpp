#pragma once

#include <thyme/core/common_structs.hpp>
#include <thyme/core/window.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <fmt/format.h>
#include <vulkan/vulkan.hpp>

#include <functional>
#include <memory>

namespace th {

class GlfwWindow: public Window {
    using WindowHWND = std::unique_ptr<GLFWwindow, std::function<void(GLFWwindow*)>>;

public:
    explicit GlfwWindow(const WindowConfig& config);
    mutable bool frameBufferResized{ false };

    void poolEvents() override {
        glfwPollEvents();
    }

    [[nodiscard]] inline bool shouldClose() noexcept override {
        return glfwWindowShouldClose(m_window.get()) != 0;
    }

    [[nodiscard]] inline auto& getHandler() const noexcept {
        return m_window;
    }

    [[nodiscard]] inline auto getFrameBufferSize() const noexcept {
        int width{};
        int height{};
        glfwGetFramebufferSize(m_window.get(), &width, &height);
        return Resolution{ .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height) };
    }

private:
    WindowHWND m_window;
};

class THYME_API VulkanGlfwWindow final: public GlfwWindow {
public:
    explicit VulkanGlfwWindow(const WindowConfig& config) : GlfwWindow(config) {}

    [[nodiscard]] static std::vector<std::string> getRequiredInstanceExtensions() noexcept;

    [[nodiscard]] auto getSurface(const vk::Instance instance) const {
        VkSurfaceKHR surface{ nullptr };
        if (const auto result = glfwCreateWindowSurface(instance, this->getHandler().get(), nullptr, &surface);
            result != VK_SUCCESS) {
            throw std::runtime_error(
                    fmt::format("GLFW cannot create VkSurface! Error: {}", static_cast<uint32_t>(result)));
        }
        return vk::UniqueSurfaceKHR(surface, instance);
    }
};

};// namespace th