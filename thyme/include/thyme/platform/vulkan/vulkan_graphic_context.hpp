#ifndef VULKAN_GRAPHIC_CONTEXT_HPP
#define VULKAN_GRAPHIC_CONTEXT_HPP

#include <vulkan/vulkan.hpp>

namespace th::vulkan {

struct VulkanGraphicContext {
    uint32_t maxFramesInFlight;
    uint32_t imageCount;
    vk::Format depthFormat;
    vk::Format colorFormat;
    vk::SurfaceFormatKHR surfaceFormat;
    vk::PresentModeKHR presentMode;
};

}// namespace th::vulkan

#endif// VULKAN_GRAPHIC_CONTEXT_HPP
