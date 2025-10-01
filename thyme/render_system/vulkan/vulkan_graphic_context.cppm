export module th.render_system.vulkan:graphic_context;

import std;

import vulkan_hpp;

namespace th {

export struct VulkanGraphicContext {
    std::uint32_t maxFramesInFlight;
    std::uint32_t imageCount;
    vk::Format depthFormat;
    vk::Format colorFormat;
    vk::SurfaceFormatKHR surfaceFormat;
    vk::PresentModeKHR presentMode;
};

}// namespace th
