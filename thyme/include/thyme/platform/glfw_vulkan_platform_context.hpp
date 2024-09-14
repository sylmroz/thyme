#pragma once

#include "thyme/core/logger.hpp"
#include "thyme/core/platform_context.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

namespace Thyme {

class THYME_API GlfwVulkanPlatformContext : public PlatformContext {
public:
    GlfwVulkanPlatformContext()
        : PlatformContext{ PlatformContextArgumenst{
                .initializer =
                        [] {
                            if (glfwInit() == GLFW_FALSE) {
                                constexpr auto message = "Failed to initialize GLFW!";
                                TH_API_LOG_ERROR(message);
                                glfwTerminate();
                                throw std::runtime_error(message);
                            }

                            if (glfwVulkanSupported() == GLFW_FALSE) {
                                auto message = "GLFW3 does not support vulkan!";
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