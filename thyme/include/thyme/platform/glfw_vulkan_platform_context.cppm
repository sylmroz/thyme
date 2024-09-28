module;

#include "thyme/export_macros.hpp"

#include "thyme/core/logger.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

export module thyme.platform.gflw_vulkan_platform_context;

import thyme.core.platform_context;

export namespace Thyme {

class THYME_API GlfwVulkanPlatformContext: public PlatformContext {
public:
    GlfwVulkanPlatformContext()
        : PlatformContext{ PlatformContextArguments{
                .initializer =
                        [] {
                            if (glfwInit() == GLFW_FALSE) {
                                constexpr auto message = "Failed to initialize GLFW!";
                                TH_API_LOG_ERROR(message);
                                glfwTerminate();
                                throw std::runtime_error(message);
                            }

                            if (glfwVulkanSupported() == GLFW_FALSE) {
                                constexpr auto message = "GLFW3 does not support vulkan!";
                                TH_API_LOG_ERROR(message);
                                glfwTerminate();
                                throw std::runtime_error(message);
                            }
                            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
                            TH_API_LOG_INFO("GLFW Vulkan platform context initialized.")
                        },
                .destroyer =
                        [] {
                            glfwTerminate();
                            TH_API_LOG_INFO("GLFW Vulkan platform context destroyed.")
                        } } } {}
};

}// namespace Thyme