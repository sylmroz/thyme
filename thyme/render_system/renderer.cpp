module;

module th.render_system.renderer;

namespace th {

Renderer::Renderer(vk::raii::PhysicalDevice& physical_device, const vk::raii::Device& device,
                   const std::uint32_t graphic_queue_index, const std::uint32_t max_frames_in_flight, Logger& logger)
    : m_command_pool(device.createCommandPool(
              vk::CommandPoolCreateInfo{ .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                         .queueFamilyIndex = graphic_queue_index })),
      m_queue(device.getQueue(graphic_queue_index, 0)),
      m_command_buffers_pool(device, m_command_pool, m_queue, max_frames_in_flight, logger) {}

void Renderer::beginFrame(const vk::raii::Device& device, const vk::Semaphore frame_semaphore) {
    m_command_buffers_pool.waitFor(device, frame_semaphore);
}

void Renderer::draw(const vk::raii::Device& device, RenderGraph& render_graph) {
    render_graph.compile();
    render_graph.execute(m_command_buffers_pool.get().getBuffer(device));
}
void Renderer::endFrame(const vk::Semaphore frame_render_semaphore) {
    m_command_buffers_pool.submit(frame_render_semaphore);
}

}// namespace th
