module;

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

export module th.platform.glfw.glfw_context;

import th.core.logger;

namespace th {

const auto terminate_handler = [](const std::string_view message) {
    glfwTerminate();
    throw std::runtime_error(message.data());
};

export class GlfwContext {
public:
    class VulkanBackend {
    public:
        VulkanBackend(Logger& logger) : m_logger{ logger } {
            m_logger.info("Initializing GLFW Vulkan...");
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

    private:
        Logger& m_logger;
    };

    GlfwContext(Logger& logger) : m_logger{ logger } {
        m_logger.info("Initializing GLFW...");
        glfwSetErrorCallback([](int error, const char* description) {
            //std::println("Error {} : msg: {}", error, description);
        });
        if (glfwInit() == GLFW_FALSE) {
            constexpr auto message = "Failed to initialize GLFW!";
            terminate_handler(message);
        }
    }

    GlfwContext(const GlfwContext&) = delete;
    GlfwContext(GlfwContext&&) = delete;
    auto operator=(const GlfwContext&) -> GlfwContext& = delete;
    auto operator=(GlfwContext&&) -> GlfwContext& = delete;

    ~GlfwContext() {
        glfwTerminate();
        m_logger.info("Terminating GLFW...");
    }

private:
    Logger& m_logger;
};

}// namespace th
