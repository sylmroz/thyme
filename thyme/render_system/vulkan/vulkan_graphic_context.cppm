export module th.render_system.vulkan:graphic_context;

import std;

import vulkan_hpp;

namespace th {

export struct VulkanGraphicContext {
    std::uint32_t max_frames_in_flight;
    std::uint32_t image_count;
    vk::Format depth_format;
    vk::Format color_format;
    vk::SurfaceFormatKHR surface_format;
    vk::PresentModeKHR present_mode;
};

}// namespace th
