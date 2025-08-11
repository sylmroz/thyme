module;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

export module th.platform.glfw.glfw_context;

import th.core.logger;
import th.platform.platform_context;

namespace th::platform::glfw {

const auto terminate_handler = [](const std::string_view message) {
    glfwTerminate();
    core::ThymeLogger().getLogger()->error("{}", message);
    throw std::runtime_error(message.data());
};

export class GlfwContext {
public:
    struct Tag{};
    class Backend : public Tag {
    public:
        Backend() {
            core::ThymeLogger().getLogger()->info("Initializing GLFW Vulkan...");
            if (glfwVulkanSupported() == GLFW_FALSE) {
                constexpr auto message = "GLFW3 does not support vulkan!";
                terminate_handler(message);
            }
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        }
    };
    GlfwContext() {
        core::ThymeLogger().getLogger()->info("Initializing GLFW...");
        glfwSetErrorCallback([](int error, const char* description) {
            core::ThymeLogger().getLogger()->error("Error{} : msg: {}", error, description);
        });
        if (glfwInit() == GLFW_FALSE) {
            constexpr auto message = "Failed to initialize GLFW!";
            terminate_handler(message);
        }
    }

    GlfwContext(const GlfwContext&) = default;
    GlfwContext(GlfwContext&&) = default;
    auto operator=(const GlfwContext&) -> GlfwContext& = default;
    auto operator=(GlfwContext&&) -> GlfwContext& = default;

    ~GlfwContext() {
        glfwTerminate();
        core::ThymeLogger().getLogger()->info("Terminating GLFW...");
    }
};

export class GlfwVulkanBackend : public GlfwContext::Tag {
public:
    GlfwVulkanBackend() {
        core::ThymeLogger().getLogger()->info("Initializing GLFW Vulkan...");
        if (glfwVulkanSupported() == GLFW_FALSE) {
            constexpr auto message = "GLFW3 does not support vulkan!";
            terminate_handler(message);
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }
};

export using GlfwVulkanPlatformContext = PlatformContext<GlfwContext>;

}// namespace th::platform::glfw
