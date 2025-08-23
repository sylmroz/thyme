module;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

#include <functional>
#include <memory>

export module th.platform.glfw.glfw_window;

import vulkan_hpp;

import th.platform.window;
import th.core.key_codes;
import th.core.logger;
import th.core.events;
import th.core.mouse_codes;
import th.platform.glfw.glfw_context;

namespace th {

class GlfwWindow: public Window {
    using WindowHWND = std::unique_ptr<GLFWwindow, std::function<void(GLFWwindow*)>>;

public:
    explicit GlfwWindow(const WindowConfig& config);

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

private:
    WindowHWND m_window;
};

export class VulkanGlfwWindow final: public GlfwWindow {
public:
    explicit VulkanGlfwWindow(const WindowConfig& config, const vk::raii::Instance& instance)
        : GlfwWindow(config), m_surface(createSurface(instance)) {}

    [[nodiscard]] static auto getExtensions() noexcept -> std::vector<std::string>;

    [[nodiscard]] auto getSurface() -> const vk::raii::SurfaceKHR& {
        return m_surface;
    }

private:
    [[nodiscard]] auto createSurface(const vk::raii::Instance& instance) const {
        VkSurfaceKHR surface{ nullptr };
        if (const auto result = glfwCreateWindowSurface(*instance, this->getHandler().get(), nullptr, &surface);
            result != VK_SUCCESS) {
            throw std::runtime_error(std::format("GLFW cannot create VkSurface! Error: {}",
                                                 vk::to_string(static_cast<vk::Result>(result))));
        }
        return vk::raii::SurfaceKHR(instance, surface);
    }

private:
    vk::raii::SurfaceKHR m_surface;
};

GlfwWindow::GlfwWindow(const WindowConfig& config) : Window{ config } {
    core::ThymeLogger().getLogger()->debug("Create window with parameters: width = {}, height = {}, name = {}",
                                           config.width,
                                           config.height,
                                           config.name);
    m_window = WindowHWND(glfwCreateWindow(static_cast<int>(config.width),
                                           static_cast<int>(config.height),
                                           config.name.data(),
                                           nullptr,
                                           nullptr),
                          [name = config.name](GLFWwindow* window) {
                              core::ThymeLogger().getLogger()->debug("Destroying window {}", name);
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
                app->m_eventListener.next(core::WindowMinimalize{});
            } else {
                app->m_eventListener.next(core::WindowMaximalize{});
            }
        }
        if (width > 0 && height > 0) {
            app->m_eventListener.next(core::WindowResize{ .width = width, .height = height });
        }
    });

    glfwSetWindowCloseCallback(m_window.get(), [](GLFWwindow* window) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->m_eventListener.next(core::WindowClose{});
    });

    glfwSetCursorPosCallback(m_window.get(), [](GLFWwindow* window, double x, double y) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->m_eventListener.next(core::MousePosition{ { x, y } });
    });

    glfwSetScrollCallback(m_window.get(), [](GLFWwindow* window, double x, double y) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        app->m_eventListener.next(core::MouseWheel{ { x, y } });
    });

    glfwSetKeyCallback(m_window.get(), [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        const auto& eventSubject = app->m_eventListener;
        if (action == GLFW_PRESS) {
            eventSubject.next(core::KeyPressed(static_cast<core::KeyCode>(key)));
        } else if (action == GLFW_RELEASE) {
            eventSubject.next(core::KeyReleased(static_cast<core::KeyCode>(key)));
        } else if (action == GLFW_REPEAT) {
            eventSubject.next(core::KeyRepeated(static_cast<core::KeyCode>(key)));
        }
    });

    glfwSetMouseButtonCallback(m_window.get(), [](GLFWwindow* window, int button, int action, int mods) {
        const auto app = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        const auto& eventSubject = app->m_eventListener;
        if (action == GLFW_PRESS) {
            eventSubject.next(core::MouseButtonPress(static_cast<core::MouseButton>(button)));
        } else if (action == GLFW_RELEASE) {
            eventSubject.next(core::MouseButtonReleased(static_cast<core::MouseButton>(button)));
        }
    });
}

auto VulkanGlfwWindow::getExtensions() noexcept -> std::vector<std::string> {
    uint32_t instanceExtensionCount{ 0 };
    auto* instanceExtensionBuffer = glfwGetRequiredInstanceExtensions(&instanceExtensionCount);
    std::vector<std::string> instanceExtensions(instanceExtensionCount);
    std::copy_n(instanceExtensionBuffer, instanceExtensionCount, instanceExtensions.begin());
    return instanceExtensions;
}

}// namespace th