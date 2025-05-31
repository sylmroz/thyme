#include <thyme/platform/vulkan/vulkan_command_buffers.hpp>

namespace th::vulkan {

VulkanCommandBuffer::VulkanCommandBuffer(const vk::Device device, const vk::CommandPool commandPool,
                                         const vk::Queue graphicQueue)
    : m_device{ device }, m_graphicQueue{ graphicQueue } {
    m_fence = m_device.createFenceUnique(vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });

    m_commandBuffer = m_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
                                                              .commandPool = commandPool,
                                                              .level = vk::CommandBufferLevel::ePrimary,
                                                              .commandBufferCount = 1,
                                                      })
                              .front();
}

void VulkanCommandBuffer::reset() {
    if (!m_submitted) {
        return;
    }
    m_submitted = false;
    if (m_device.waitForFences({ m_fence.get() }, vk::True, std::numeric_limits<uint64_t>::max())
        != vk::Result::eSuccess) {
        TH_API_LOG_ERROR("Failed to wait for a complete fence");
        throw std::runtime_error("Failed to wait for a complete fence");
    }
    m_device.resetFences({ m_fence.get() });
    m_dependSemaphores.clear();
    m_commandBuffer.reset();
}

void VulkanCommandBuffer::submit(const vk::PipelineStageFlags stage, const vk::Semaphore renderSemaphore) {
    m_submitted = true;
    m_commandBuffer.end();
    const auto dependants = m_dependSemaphores | std::views::transform([](auto& semaphore) { return semaphore.get(); })
                            | std::ranges::to<std::vector>();
    m_graphicQueue.submit(
            vk::SubmitInfo{
                    .waitSemaphoreCount = static_cast<uint32_t>(m_dependSemaphores.size()),
                    .pWaitSemaphores = dependants.data(),
                    .pWaitDstStageMask = &stage,
                    .commandBufferCount = 1,
                    .pCommandBuffers = &m_commandBuffer,
                    .signalSemaphoreCount = 1,
                    .pSignalSemaphores = &renderSemaphore,
            },
            m_fence.get());
}

VulkanCommandBuffersPool::VulkanCommandBuffersPool(const vk::Device device, const vk::CommandPool commandPool,
                                                   const vk::Queue graphicQueue, const std::size_t capacity)
    : m_device{ device } {
    std::generate_n(std::back_inserter(m_commandBuffers), capacity, [device, commandPool, graphicQueue]() {
        return VulkanCommandBuffer{ device, commandPool, graphicQueue };
    });
}

}// namespace th::vulkan