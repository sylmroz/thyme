export module th.platform.glfw.glfw_window:glfw_context;

import std;

import glfw;

import th.core.logger;

namespace th {

const auto terminate_handler = [](const std::string_view message) {
    glfwTerminate();
    throw std::runtime_error(message.data());
};

export class GlfwContext {
public:
    explicit GlfwContext(Logger& logger) : m_logger{ logger } {
        m_logger.info("Initializing GLFW...");
        glfwSetErrorCallback([](int error, const char* description) {
            //std::println("Error {} : msg: {}", error, description);
        });
        if (glfwInit() == glfw_false) {
            constexpr auto message = "Failed to initialize GLFW!";
            terminate_handler(message);
        }
        if (glfwVulkanSupported() == glfw_false) {
            constexpr auto message = "GLFW3 does not support vulkan!";
            terminate_handler(message);
        }
        glfwWindowHint(glfw_client_api, glfw_no_api);
    }

    GlfwContext(const GlfwContext&) = delete;
    GlfwContext(GlfwContext&&) = delete;
    auto operator=(const GlfwContext&) -> GlfwContext& = delete;
    auto operator=(GlfwContext&&) -> GlfwContext& = delete;

    [[nodiscard]] static auto getExtensions() noexcept -> std::vector<std::string> {
        uint32_t instanceExtensionCount{ 0 };
        auto* instanceExtensionBuffer = glfwGetRequiredInstanceExtensions(&instanceExtensionCount);
        std::vector<std::string> instanceExtensions(instanceExtensionCount);
        std::copy_n(instanceExtensionBuffer, instanceExtensionCount, instanceExtensions.begin());
        return instanceExtensions;
    }

    ~GlfwContext() {
        glfwTerminate();
        m_logger.info("Terminating GLFW...");
    }

private:
    Logger& m_logger;
};

}// namespace th
