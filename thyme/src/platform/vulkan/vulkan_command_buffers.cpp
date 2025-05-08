#include <thyme/platform/vulkan/vulkan_command_buffers.hpp>

namespace th::vulkan {

VulkanCommandBuffer::VulkanCommandBuffer(const vk::Device device, const vk::CommandPool commandPool,
                                         const vk::Queue graphicQueue)
    : m_device{ device }, m_commandPool{ commandPool }, m_graphicQueue{ graphicQueue } {
    m_fence = m_device.createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    m_commandBuffer = m_device.allocateCommandBuffers(
                                      vk::CommandBufferAllocateInfo(m_commandPool, vk::CommandBufferLevel::ePrimary, 1))
                              .front();
    m_semaphore = m_device.createSemaphoreUnique(vk::SemaphoreCreateInfo());
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

void VulkanCommandBuffer::start() const {
    m_commandBuffer.begin(vk::CommandBufferBeginInfo());
}

vk::Semaphore VulkanCommandBuffer::submit(const vk::PipelineStageFlags stage) {
    m_submitted = true;
    m_commandBuffer.end();
    const auto dependants = m_dependSemaphores | std::views::transform([](auto& semaphore) { return semaphore.get(); })
                            | std::ranges::to<std::vector>();
    const auto submitInfo = vk::SubmitInfo(dependants, { stage }, { m_commandBuffer }, { m_semaphore.get() });
    m_graphicQueue.submit(submitInfo, m_fence.get());
    return m_semaphore.get();
}

VulkanCommandBuffersPool::VulkanCommandBuffersPool(const vk::Device device, const vk::CommandPool commandPool,
                                                   const vk::Queue graphicQueue, const std::size_t capacity) {
    std::generate_n(std::back_inserter(m_commandBuffers), capacity, [device, commandPool, graphicQueue]() {
        return VulkanCommandBuffer{ device, commandPool, graphicQueue };
    });
}

}// namespace th::vulkan