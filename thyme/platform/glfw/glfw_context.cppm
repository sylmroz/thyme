export module th.platform.glfw.glfw_window:glfw_context;

import std;

import glfw;

import th.core.logger;

namespace th {

const auto terminate_handler = [](const std::string_view message) -> void {
    glfwTerminate();
    throw std::runtime_error(message.data());
};

export class GlfwContext {
public:
    explicit GlfwContext(Logger& logger) : m_logger{ logger } {
        m_logger.info("Initializing GLFW...");
        glfwSetErrorCallback([]([[maybe_unused]] int error, [[maybe_unused]] const char* description) -> void {
            // std::println("Error {} : msg: {}", error, description);
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
        uint32_t instance_extension_count{ 0 };
        auto* instance_extension_buffer = glfwGetRequiredInstanceExtensions(&instance_extension_count);
        std::vector<std::string> instance_extensions(instance_extension_count);
        std::copy_n(instance_extension_buffer, instance_extension_count, instance_extensions.begin());
        return instance_extensions;
    }

    ~GlfwContext() {
        glfwTerminate();
        m_logger.info("Terminating GLFW...");
    }

private:
    Logger& m_logger;
};

}// namespace th
