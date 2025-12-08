module;

module th.render_system.vulkan;

import std;

import th.core.logger;

namespace th {

VulkanCommandBuffer::VulkanCommandBuffer(const vk::raii::Device& device, const vk::CommandPool command_pool,
                                         const vk::Queue graphic_queue, Logger& logger)
    : m_logger{ logger }, m_graphic_queue{ graphic_queue },
      m_command_buffer{ std::move(device.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
                                                                .commandPool = command_pool,
                                                                .level = vk::CommandBufferLevel::ePrimary,
                                                                .commandBufferCount = 1u,
                                                        })
                                .front()) },
      m_fence{ device, vk::FenceCreateInfo{} } {}

auto VulkanCommandBuffer::getBuffer(const vk::raii::Device& device) -> vk::CommandBuffer {
    start(device);
    return m_command_buffer;
}

void VulkanCommandBuffer::reset(const vk::raii::Device& device) {
    if (m_state == State::Submitted) {
        if (device.waitForFences({ m_fence }, vk::True, std::numeric_limits<uint64_t>::max())
            != vk::Result::eSuccess) {
            m_logger.error("Failed to wait for a complete fence");
            throw std::runtime_error("Failed to wait for a complete fence");
        }
        device.resetFences({ m_fence });
    }
    m_depend_semaphores.clear();
    m_command_buffer.reset();
    m_state = State::Idle;
}

void VulkanCommandBuffer::start(const vk::raii::Device& device) {
    if (m_state == State::Recording) {
        return;
    }
    if (m_state == State::Submitted) {
        reset(device);
    }

    m_command_buffer.begin(vk::CommandBufferBeginInfo());
    m_state = State::Recording;
}

void VulkanCommandBuffer::submit(const vk::PipelineStageFlags stage, const vk::Semaphore render_semaphore) {
    if (m_state != State::Recording) {
        return;
    }
    m_command_buffer.end();
    const auto dependants = m_depend_semaphores | std::views::transform([](auto& semaphore) { return *semaphore; })
                            | std::ranges::to<std::vector>();
    m_graphic_queue.submit(
            vk::SubmitInfo{
                    .waitSemaphoreCount = static_cast<uint32_t>(m_depend_semaphores.size()),
                    .pWaitSemaphores = dependants.data(),
                    .pWaitDstStageMask = &stage,
                    .commandBufferCount = 1u,
                    .pCommandBuffers = &(*m_command_buffer),
                    .signalSemaphoreCount = 1u,
                    .pSignalSemaphores = &render_semaphore,
            },
            m_fence);
    m_state = State::Submitted;
}

void VulkanCommandBuffer::waitFor(const vk::raii::Device& device, vk::raii::Semaphore depend_semaphore) {
    start(device);
    m_depend_semaphores.emplace_back(std::move(depend_semaphore));
}

VulkanCommandBuffersPool::VulkanCommandBuffersPool(const vk::raii::Device& device, const vk::CommandPool command_pool,
                                                   const vk::Queue graphic_queue, const std::size_t capacity,
                                                   Logger& logger) {
    std::generate_n(std::back_inserter(m_command_buffers), capacity, [&device, command_pool, graphic_queue, &logger]() mutable {
        return VulkanCommandBuffer{ device, command_pool, graphic_queue, logger };
    });
}

}// namespace th