module;

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

export module th.platform.glfw.glfw_context;

import th.core.logger;
import th.platform.platform_context;

namespace th {

const auto terminate_handler = [](const std::string_view message) {
    glfwTerminate();
    ThymeLogger().getLogger()->error("{}", message);
    throw std::runtime_error(message.data());
};

export class GlfwContext {
public:
    class VulkanBackend {
    public:
        VulkanBackend() {
            ThymeLogger().getLogger()->info("Initializing GLFW Vulkan...");
            if (glfwVulkanSupported() == GLFW_FALSE) {
                constexpr auto message = "GLFW3 does not support vulkan!";
                terminate_handler(message);
            }
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        }

        [[nodiscard]] static auto getExtensions() noexcept -> std::vector<std::string> {
            uint32_t instanceExtensionCount{ 0 };
            auto* instanceExtensionBuffer = glfwGetRequiredInstanceExtensions(&instanceExtensionCount);
            std::vector<std::string> instanceExtensions(instanceExtensionCount);
            std::copy_n(instanceExtensionBuffer, instanceExtensionCount, instanceExtensions.begin());
            return instanceExtensions;
        }
    };
    GlfwContext() {
        ThymeLogger().getLogger()->info("Initializing GLFW...");
        glfwSetErrorCallback([](int error, const char* description) {
            ThymeLogger().getLogger()->error("Error {} : msg: {}", error, description);
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
        ThymeLogger().getLogger()->info("Terminating GLFW...");
    }
};

export using GlfwVulkanPlatformContext = VulkanBackendPlatformContext<GlfwContext>;

}// namespace th
