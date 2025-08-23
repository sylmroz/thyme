module;

export module th.render_system.vulkan.graphic_context;

import vulkan_hpp;

namespace th {

export struct VulkanGraphicContext {
    uint32_t maxFramesInFlight;
    uint32_t imageCount;
    vk::Format depthFormat;
    vk::Format colorFormat;
    vk::SurfaceFormatKHR surfaceFormat;
    vk::PresentModeKHR presentMode;
};

}// namespace th
