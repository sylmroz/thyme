module;

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <functional>
#include <memory>
#include <spdlog/logger.h>

export module th.platform.glfw.glfw_window;

import :glfw_context;

import vulkan_hpp;

import th.platform.window;
import th.core.key_codes;
import th.core.logger;
import th.core.events;
import th.core.mouse_codes;

namespace th {

export class GlfwWindow final: public Window {
    using WindowHWND = std::unique_ptr<GLFWwindow, std::function<void(GLFWwindow*)>>;

public:
    explicit GlfwWindow(const WindowConfig& config, Logger& logger);

    void poolEvents() override {
        glfwPollEvents();
    }

    [[nodiscard]] bool shouldClose() noexcept override {
        return glfwWindowShouldClose(m_window.get()) != 0;
    }

    [[nodiscard]] auto& getHandler() const noexcept {
        return m_window;
    }

    [[nodiscard]] auto getFrameBufferSize() const noexcept -> glm::uvec2 {
        int width{};
        int height{};
        glfwGetFramebufferSize(m_window.get(), &width, &height);
        return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    }

    void initializeContext() {
        [[maybe_unused]] static auto glfwContext = GlfwContext(m_logger);
    }

    [[nodiscard]] auto createSurface(const vk::raii::Instance& instance) const -> vk::raii::SurfaceKHR {
        VkSurfaceKHR surface{ nullptr };
        if (const auto result = glfwCreateWindowSurface(*instance, this->getHandler().get(), nullptr, &surface);
            result != VK_SUCCESS) {
            throw std::runtime_error(std::format("GLFW cannot create VkSurface! Error: {}",
                                                 vk::to_string(static_cast<vk::Result>(result))));
        }
        return vk::raii::SurfaceKHR(instance, surface);
    }

    [[nodiscard]] static auto getExtensions() noexcept -> std::vector<std::string> {
        return GlfwContext::getExtensions();
    }

private:
    WindowHWND m_window;
};

}// namespace th