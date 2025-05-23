#pragma once

#include "thyme/export_macros.hpp"

#include "thyme/core/logger.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

#include <thyme/core/platform_context.hpp>

namespace th {

class THYME_API GlfwVulkanPlatformContext: public PlatformContext {
public:
    GlfwVulkanPlatformContext()
        : PlatformContext{ PlatformContextArguments{
                  .initializer =
                          [] {
                              const auto terminate_handler = [](const std::string_view message) {
                                  glfwTerminate();
                                  TH_API_LOG_ERROR(message);
                                  throw std::runtime_error(message.data());
                              };
                              if (glfwInit() == GLFW_FALSE) {
                                  constexpr auto message = "Failed to initialize GLFW!";
                                  terminate_handler(message);
                              }

                              if (glfwVulkanSupported() == GLFW_FALSE) {
                                  constexpr auto message = "GLFW3 does not support vulkan!";
                                  terminate_handler(message);
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

}// namespace th