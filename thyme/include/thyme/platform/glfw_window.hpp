#pragma once

#include "thyme/core/window.hpp"

#include <GLFW/glfw3.h>

#include <memory>

namespace Thyme {

class GlfwWindow : public Window {
    using WindowHWND = std::unique_ptr<GLFWwindow, std::function<void(GLFWwindow*)>>;
public:
    GlfwWindow(const WindowConfiguration& config);
    virtual void poolEvents() override;
    virtual bool shouldClose() override;

private:
    WindowHWND m_window;
};

};