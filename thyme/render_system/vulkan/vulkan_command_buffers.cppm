module;

#include <vector>

export module th.render_system.vulkan:command_buffers;

import vulkan_hpp;

namespace th {

export class VulkanCommandBuffer {
    enum class State {
        Idle,
        Recording,
        Submitted
    };

public:
    VulkanCommandBuffer(vk::Device device, vk::CommandPool commandPool, vk::Queue graphicQueue);

    [[nodiscard]] auto getBuffer() -> vk::CommandBuffer;

    [[nodiscard]] auto getFence() const -> vk::Fence {
        return m_fence.get();
    }

    void reset();
    void start();
    void submit(vk::PipelineStageFlags stage, vk::Semaphore renderSemaphore);
    void waitFor(vk::UniqueSemaphore& dependSemaphore);

    [[nodiscard]] auto isSubmitted() const -> bool {
        return m_state == State::Submitted;
    }

private:
    State m_state{ State::Idle };

    vk::Device m_device;
    vk::Queue m_graphicQueue;

    vk::CommandBuffer m_commandBuffer;
    vk::UniqueFence m_fence;
    vk::UniqueSemaphore m_semaphore;
    std::vector<vk::UniqueSemaphore> m_dependSemaphores;
};

export class VulkanCommandBuffersPool {
public:
    VulkanCommandBuffersPool(vk::Device device, vk::CommandPool commandPool, vk::Queue graphicQueue, std::size_t capacity);
    [[nodiscard]] auto get() -> VulkanCommandBuffer& {
        auto& currentBuffer = m_commandBuffers[m_current];
        return currentBuffer;
    }

    void waitFor(vk::UniqueSemaphore& dependSemaphore) {
        auto& current = get();
        current.waitFor(dependSemaphore);
    }

    void submit(const vk::Semaphore renderSemaphore) {
        get().submit(vk::PipelineStageFlagBits::eColorAttachmentOutput, renderSemaphore);
        m_current = (m_current + 1) % m_commandBuffers.size();
    }

    void flush() {
        for (auto& buffer : m_commandBuffers) {
            buffer.reset();
        }
    }

private:
    std::size_t m_current{ 0 };
    vk::Device m_device;
    std::vector<VulkanCommandBuffer> m_commandBuffers;
};

}// namespace th::render_system::vulkan
