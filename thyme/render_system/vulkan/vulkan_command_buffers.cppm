export module th.render_system.vulkan:command_buffers;

import std;

import vulkan_hpp;

import th.core.logger;

namespace th {

export class VulkanCommandBuffer {
    enum class State {
        Idle,
        Recording,
        Submitted
    };

public:
    VulkanCommandBuffer(vk::Device device, vk::CommandPool command_pool, vk::Queue graphic_queue, Logger& logger);

    [[nodiscard]] auto getBuffer() -> vk::CommandBuffer;

    [[nodiscard]] auto getFence() const -> vk::Fence {
        return m_fence.get();
    }

    void reset();
    void start();
    void submit(vk::PipelineStageFlags stage, vk::Semaphore render_semaphore);
    void waitFor(vk::UniqueSemaphore& depend_semaphore);

    [[nodiscard]] auto isSubmitted() const -> bool {
        return m_state == State::Submitted;
    }

private:
    State m_state{ State::Idle };

    Logger& m_logger;

    vk::Device m_device;
    vk::Queue m_graphic_queue;

    vk::CommandBuffer m_command_buffer;
    vk::UniqueFence m_fence;
    vk::UniqueSemaphore m_semaphore;
    std::vector<vk::UniqueSemaphore> m_depend_semaphores;
};

export class VulkanCommandBuffersPool {
public:
    VulkanCommandBuffersPool(vk::Device device, vk::CommandPool command_pool, vk::Queue graphic_queue,
                             std::size_t capacity, Logger& logger);
    [[nodiscard]] auto get() -> VulkanCommandBuffer& {
        auto& current_buffer = m_command_buffers[m_current];
        return current_buffer;
    }

    void waitFor(vk::UniqueSemaphore& depend_semaphore) {
        auto& current = get();
        current.waitFor(depend_semaphore);
    }

    void submit(const vk::Semaphore render_semaphore) {
        get().submit(vk::PipelineStageFlagBits::eColorAttachmentOutput, render_semaphore);
        m_current = (m_current + 1) % m_command_buffers.size();
    }

    void flush() {
        for (auto& buffer : m_command_buffers) {
            buffer.reset();
        }
    }

private:
    std::size_t m_current{ 0 };
    vk::Device m_device;
    std::vector<VulkanCommandBuffer> m_command_buffers;
};

}// namespace th
