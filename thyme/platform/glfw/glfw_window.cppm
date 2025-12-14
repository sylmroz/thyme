export module th.platform.glfw.glfw_window;

import std;

import glfw;
import glm;
import vulkan;

import th.platform.window;
import th.platform.window_event_handler;
import th.core.key_codes;
import th.core.logger;
import th.core.events;
import th.core.mouse_codes;

import :glfw_context;

namespace th {

export class GlfwWindow final: public Window {
    using WindowHWND = std::unique_ptr<GLFWwindow, std::function<void(GLFWwindow*)>>;

public:
    explicit GlfwWindow(const WindowConfig& config, WindowEventsHandlers& event_handlers, Logger& logger);

    void poolEvents() override {
        glfwPollEvents();
    }

    [[nodiscard]] auto shouldClose() noexcept -> bool override {
        return glfwWindowShouldClose(m_window.get()) != 0;
    }

    [[nodiscard]] auto getHandler() const noexcept -> auto& {
        return m_window;
    }

    [[nodiscard]] auto getFrameBufferSize() const noexcept -> glm::uvec2 {
        int width{};
        int height{};
        glfwGetFramebufferSize(m_window.get(), &width, &height);
        return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    }

    void initializeContext() const {
        [[maybe_unused]] static auto glfw_context = GlfwContext(m_logger);
    }

    [[nodiscard]] auto createSurface(const vk::raii::Instance& instance) const -> vk::raii::SurfaceKHR;

    [[nodiscard]] static auto getExtensions() noexcept -> std::vector<std::string> {
        return GlfwContext::getExtensions();
    }

private:
    WindowHWND m_window;
};

}// namespace th