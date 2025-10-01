module;

#include <GLFW/glfw3.h>

export module glfw;

export {
    using ::GLFWwindow;

    using ::glfwGetFramebufferSize;
    using ::glfwWindowHint;
    using ::glfwCreateWindow;
    using ::glfwDestroyWindow;
    using ::glfwGetWindowUserPointer;
    using ::glfwSetWindowUserPointer;
    using ::glfwCreateWindowSurface;
    using ::glfwTerminate;
    using ::glfwSetErrorCallback;
    using ::glfwInit;
    using ::glfwVulkanSupported;
    using ::glfwGetRequiredInstanceExtensions;

    using ::glfwPollEvents;
    using ::glfwWindowShouldClose;

    using ::glfwSetFramebufferSizeCallback;
    using ::glfwSetWindowCloseCallback;
    using ::glfwSetCursorPosCallback;
    using ::glfwSetScrollCallback;
    using ::glfwSetKeyCallback;
    using ::glfwSetMouseButtonCallback;
    using ::glfwSetWindowMaximizeCallback;

    constexpr auto glfw_maximized = GLFW_MAXIMIZED;
    constexpr auto glfw_decorate = GLFW_DECORATED;

    constexpr auto glfw_press = GLFW_PRESS;
    constexpr auto glfw_release = GLFW_RELEASE;
    constexpr auto glfw_repeat = GLFW_REPEAT;

    constexpr auto glfw_true = GLFW_TRUE;
    constexpr auto glfw_false = GLFW_FALSE;

    constexpr auto glfw_client_api = GLFW_CLIENT_API;
    constexpr auto glfw_no_api = GLFW_NO_API;
}
