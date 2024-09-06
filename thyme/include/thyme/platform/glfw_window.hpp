#pragma once

#include "thyme/core/logger.hpp"
#include "thyme/core/window.hpp"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <memory>

namespace Thyme {

class EmptyContext {
public:
    void init(){};
};

class GlfwVulkanContext {
public:
    void init() {
        if (glfwVulkanSupported() == GLFW_FALSE) {
            auto message = "GLFW3 does not support vulkan!";
            TH_API_LOG_ERROR(message);
            glfwTerminate();
            throw std::runtime_error(message);
        }
    }

    std::vector<std::string> getRequiredInstanceExtensions() {
        uint32_t instanceExtensionCount{ 0 };
        auto instanceExtensionBuffer = glfwGetRequiredInstanceExtensions(&instanceExtensionCount);
        std::vector<std::string> instanceExtension;
        instanceExtension.reserve(instanceExtensionCount);
        for (uint32_t i{ 0 }; i < instanceExtensionCount; ++i) {
            instanceExtension.emplace_back(instanceExtensionBuffer[i]);
        }
        return instanceExtension;
    }
};

template<typename Context = EmptyContext>
class GlfwWindow
    : public Window
    , public Context {
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

    WindowHWND m_window;
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
    Context::init();
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

using VulkanGlfwWindow = GlfwWindow<GlfwVulkanContext>;

};// namespace Thyme