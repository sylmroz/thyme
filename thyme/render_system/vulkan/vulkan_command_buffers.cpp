module;

module th.render_system.vulkan;

import std;

import th.core.logger;

namespace th {

VulkanCommandBuffer::VulkanCommandBuffer(const vk::Device device, const vk::CommandPool command_pool,
                                         const vk::Queue graphic_queue, Logger& logger)
    : m_logger{ logger }, m_device{ device }, m_graphic_queue{ graphic_queue },
      m_command_buffer{ m_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
                                                                .commandPool = command_pool,
                                                                .level = vk::CommandBufferLevel::ePrimary,
                                                                .commandBufferCount = 1u,
                                                        })
                                .front() },
      m_fence{ m_device.createFenceUnique(vk::FenceCreateInfo{}) } {}

auto VulkanCommandBuffer::getBuffer() -> vk::CommandBuffer {
    start();
    return m_command_buffer;
}

void VulkanCommandBuffer::reset() {
    if (m_state == State::Submitted) {
        if (m_device.waitForFences({ m_fence.get() }, vk::True, std::numeric_limits<uint64_t>::max())
            != vk::Result::eSuccess) {
            m_logger.error("Failed to wait for a complete fence");
            throw std::runtime_error("Failed to wait for a complete fence");
        }
        m_device.resetFences({ m_fence.get() });
    }
    m_depend_semaphores.clear();
    m_command_buffer.reset();
    m_state = State::Idle;
}

void VulkanCommandBuffer::start() {
    if (m_state == State::Recording) {
        return;
    }
    if (m_state == State::Submitted) {
        reset();
    }

    m_command_buffer.begin(vk::CommandBufferBeginInfo());
    m_state = State::Recording;
}

void VulkanCommandBuffer::submit(const vk::PipelineStageFlags stage, const vk::Semaphore render_semaphore) {
    if (m_state != State::Recording) {
        return;
    }
    m_command_buffer.end();
    const auto dependants = m_depend_semaphores | std::views::transform([](auto& semaphore) { return semaphore.get(); })
                            | std::ranges::to<std::vector>();
    m_graphic_queue.submit(
            vk::SubmitInfo{
                    .waitSemaphoreCount = static_cast<uint32_t>(m_depend_semaphores.size()),
                    .pWaitSemaphores = dependants.data(),
                    .pWaitDstStageMask = &stage,
                    .commandBufferCount = 1u,
                    .pCommandBuffers = &m_command_buffer,
                    .signalSemaphoreCount = 1u,
                    .pSignalSemaphores = &render_semaphore,
            },
            m_fence.get());
    m_state = State::Submitted;
}

void VulkanCommandBuffer::waitFor(vk::UniqueSemaphore& depend_semaphore) {
    start();
    m_depend_semaphores.emplace_back(std::move(depend_semaphore));
}

VulkanCommandBuffersPool::VulkanCommandBuffersPool(const vk::Device device, const vk::CommandPool command_pool,
                                                   const vk::Queue graphic_queue, const std::size_t capacity,
                                                   Logger& logger)
    : m_device{ device } {
    std::generate_n(std::back_inserter(m_command_buffers), capacity, [device, command_pool, graphic_queue, &logger]() {
        return VulkanCommandBuffer{ device, command_pool, graphic_queue, logger };
    });
}

}// namespace th