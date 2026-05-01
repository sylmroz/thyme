export module th.render_system.renderer;

import std;
import vulkan;

import th.render_system.vulkan;
import th.core.logger;
import th.render_system.render_graph;

namespace th {

export class Renderer {
public:
    Renderer(vk::raii::PhysicalDevice& physical_device, const vk::raii::Device& device, std::uint32_t graphic_queue_index,
             std::uint32_t max_frames_in_flight, Logger& logger);

    void beginFrame(const vk::raii::Device& device, vk::Semaphore frame_semaphore);
    void draw(const vk::raii::Device& device, RenderGraph& render_graph);
    void endFrame(vk::Semaphore frame_render_semaphore);

private:
    vk::raii::CommandPool m_command_pool;
    vk::raii::Queue m_queue;
    VulkanCommandBuffersPool2 m_command_buffers_pool;
};


}// namespace th
