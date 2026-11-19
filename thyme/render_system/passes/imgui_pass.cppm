export module th.render_system.passes:imgui_pass;

import std;

import imgui;
import vulkan;

import th.core.logger;
import th.render_system.render_graph;

namespace th {

export class ImGuiPass {
public:
    ImGuiPass([[maybe_unused]] vk::raii::PhysicalDevice& physical_device, const vk::raii::Device& device,
           const vk::Format format, std::span<const vk::DescriptorBufferInfo> camera_descriptor_buffer_info,
           const Logger& logger);

    void setup(RenderGraph& render_graph) const;
};

}
