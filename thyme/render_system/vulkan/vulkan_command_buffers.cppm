export module th.render_system.vulkan:command_buffers;

import std;

import vulkan;

import th.core.logger;

namespace th {

export class VulkanCommandBuffer {
    enum class State {
        Idle,
        Recording,
        Submitted
    };

public:
    VulkanCommandBuffer(const vk::raii::Device& device, vk::CommandPool command_pool, vk::Queue graphic_queue, Logger& logger);

    [[nodiscard]] auto getBuffer(const vk::raii::Device& device) -> vk::CommandBuffer;

    [[nodiscard]] auto getFence() const -> vk::Fence {
        return m_fence;
    }

    void reset(const vk::raii::Device& device);
    void start(const vk::raii::Device& device);
    void submit(vk::PipelineStageFlags stage, vk::Semaphore render_semaphore);
    void waitFor(const vk::raii::Device& device, vk::raii::Semaphore depend_semaphore);

    [[nodiscard]] auto isSubmitted() const -> bool {
        return m_state == State::Submitted;
    }

private:
    State m_state{ State::Idle };

    Logger& m_logger;

    vk::Queue m_graphic_queue;

    vk::raii::CommandBuffer m_command_buffer;
    vk::raii::Fence m_fence;
    std::vector<vk::raii::Semaphore> m_depend_semaphores;
};

export class VulkanCommandBuffersPool {
public:
    VulkanCommandBuffersPool(const vk::raii::Device& device, vk::CommandPool command_pool, vk::Queue graphic_queue,
                             std::size_t capacity, Logger& logger);
    [[nodiscard]] auto get() -> VulkanCommandBuffer& {
        auto& current_buffer = m_command_buffers[m_current];
        return current_buffer;
    }

    void waitFor(const vk::raii::Device& device, vk::raii::Semaphore depend_semaphore) {
        auto& current = get();
        current.waitFor(device, std::move(depend_semaphore));
    }

    void submit(const vk::Semaphore render_semaphore) {
        get().submit(vk::PipelineStageFlagBits::eColorAttachmentOutput, render_semaphore);
        m_current = (m_current + 1) % m_command_buffers.size();
    }

    void flush(const vk::raii::Device& device) {
        for (auto& buffer : m_command_buffers) {
            buffer.reset(device);
        }
    }

private:
    std::size_t m_current{ 0 };
    std::vector<VulkanCommandBuffer> m_command_buffers;
};

}// namespace th
