module;

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <functional>
#include <memory>
#include <spdlog/logger.h>

export module th.platform.glfw.glfw_window;

import vulkan_hpp;

import th.platform.window;
import th.core.key_codes;
import th.core.logger;
import th.core.events;
import th.core.mouse_codes;
import th.platform.glfw.glfw_context;

namespace th {

template <typename Backend>
class GlfwWindow: public Window {
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
        static_cast<Backend&>(*this).initializeBackendContext();
    }

private:
    WindowHWND m_window;
};

template <typename Backend>
GlfwWindow<Backend>::GlfwWindow(const WindowConfig& config, Logger& logger) : Window{ config, logger } {
    logger.debug("Create window with parameters: width = {}, height = {}, name = {}",
                                           config.width,
                                           config.height,
                                           config.name);
    initializeContext();
    glfwWindowHint(GLFW_MAXIMIZED, config.maximalized);
    glfwWindowHint(GLFW_DECORATED, config.decorate);
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
        const auto state = (width == 0 || height == 0) ? WindowState::minimalized : WindowState::maximalized;
        if (state != app->m_windowState) {
            app->m_windowState = state;
            if (state == WindowState::minimalized) {
                app->m_eventListener.next(WindowMinimalize{});
            } else {
                app->m_eventListener.next(WindowMaximalize{});
            }
        }
        if (width > 0 && height > 0) {
            app->m_eventListener.next(WindowResize{ .width = width, .height = height });
        }
    });

    glfwSetWindowCloseCallback(m_window.get(), [](GLFWwindow* window) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->m_eventListener.next(WindowClose{});
    });

    glfwSetCursorPosCallback(m_window.get(), [](GLFWwindow* window, double x, double y) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->m_eventListener.next(MousePosition{ { x, y } });
    });

    glfwSetScrollCallback(m_window.get(), [](GLFWwindow* window, double x, double y) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->m_eventListener.next(MouseWheel{ { x, y } });
    });

    glfwSetKeyCallback(m_window.get(), [](GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        const auto& eventSubject = app->m_eventListener;
        if (action == GLFW_PRESS) {
            eventSubject.next(KeyPressed(static_cast<KeyCode>(key)));
        } else if (action == GLFW_RELEASE) {
            eventSubject.next(KeyReleased(static_cast<KeyCode>(key)));
        } else if (action == GLFW_REPEAT) {
            eventSubject.next(KeyRepeated(static_cast<KeyCode>(key)));
        }
    });

    glfwSetMouseButtonCallback(m_window.get(), [](GLFWwindow* window, int button, int action, [[maybe_unused]] int mods) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        const auto& eventSubject = app->m_eventListener;
        if (action == GLFW_PRESS) {
            eventSubject.next(MouseButtonPress(static_cast<MouseButton>(button)));
        } else if (action == GLFW_RELEASE) {
            eventSubject.next(MouseButtonReleased(static_cast<MouseButton>(button)));
        }
    });

    glfwSetWindowMaximizeCallback(m_window.get(), [](GLFWwindow* window, int maximize) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        if (maximize == GLFW_TRUE) {
            app->m_eventListener.next(WindowMaximalize{});
        } else {
            app->m_eventListener.next(WindowMinimalize{});
        }
    });
}

export class VulkanGlfwWindow final: public GlfwWindow<VulkanGlfwWindow> {
public:
    explicit VulkanGlfwWindow(const WindowConfig& config, Logger& logger)
        : GlfwWindow(config, logger) {}

    [[nodiscard]] auto createSurface(const vk::raii::Instance& instance) const -> vk::raii::SurfaceKHR {
        VkSurfaceKHR surface{ nullptr };
        if (const auto result = glfwCreateWindowSurface(*instance, this->getHandler().get(), nullptr, &surface);
            result != VK_SUCCESS) {
            throw std::runtime_error(std::format("GLFW cannot create VkSurface! Error: {}",
                                                 vk::to_string(static_cast<vk::Result>(result))));
            }
        return vk::raii::SurfaceKHR(instance, surface);
    }

    void initializeBackendContext() const {
        [[maybe_unused]] static auto vulkanBackend = GlfwContext::VulkanBackend(m_logger);
    }

    [[nodiscard]] static auto getExtensions() noexcept -> std::vector<std::string> {
        return GlfwContext::VulkanBackend::getExtensions();
    }
};

}// namespace th