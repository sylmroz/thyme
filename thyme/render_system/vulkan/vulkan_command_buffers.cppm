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
    VulkanCommandBuffer(const vk::raii::Device& device, vk::CommandPool command_pool, vk::Queue queue, Logger& logger);

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

    vk::Queue m_queue;

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

    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return m_command_buffers.size();
    }

    [[nodiscard]] auto currentIndex() const noexcept -> std::size_t {
        return m_current;
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

export class VulkanCommandBuffer2 {
    enum class State {
        Idle,
        Recording,
        Submitted
    };

public:
    VulkanCommandBuffer2(const vk::raii::Device& device, vk::CommandPool command_pool, vk::Queue queue, Logger& logger);

    [[nodiscard]] auto getBuffer(const vk::raii::Device& device) -> vk::CommandBuffer;

    [[nodiscard]] auto getFence() const -> vk::Fence {
        return m_fence;
    }

    void reset(const vk::raii::Device& device);
    void start(const vk::raii::Device& device);
    void submit(vk::PipelineStageFlags stage, vk::Semaphore semaphore);
    void waitFor(const vk::raii::Device& device, vk::Semaphore depend_semaphore);

    [[nodiscard]] auto isSubmitted() const -> bool {
        return m_state == State::Submitted;
    }

private:
    State m_state{ State::Idle };

    Logger& m_logger;

    vk::Queue m_queue;

    vk::raii::CommandBuffer m_command_buffer;
    vk::raii::Fence m_fence;
    std::vector<vk::Semaphore> m_depend_semaphores;
};

export class VulkanCommandBuffersPool2 {
public:
    VulkanCommandBuffersPool2(const vk::raii::Device& device, vk::CommandPool command_pool, vk::Queue graphic_queue,
                             std::size_t capacity, Logger& logger);
    [[nodiscard]] auto get() -> VulkanCommandBuffer2& {
        auto& current_buffer = m_command_buffers[m_current];
        return current_buffer;
    }

    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return m_command_buffers.size();
    }

    [[nodiscard]] auto currentIndex() const noexcept -> std::size_t {
        return m_current;
    }

    void waitFor(const vk::raii::Device& device, const vk::Semaphore depend_semaphore) {
        auto& current = get();
        current.waitFor(device, depend_semaphore);
    }

     void submit(const vk::Semaphore semaphore) {
        get().submit(vk::PipelineStageFlagBits::eColorAttachmentOutput, semaphore);
        m_current = (m_current + 1) % m_command_buffers.size();
    }

    void flush(const vk::raii::Device& device) {
        for (auto& buffer : m_command_buffers) {
            buffer.reset(device);
        }
    }

private:
    std::size_t m_current{ 0 };
    std::vector<VulkanCommandBuffer2> m_command_buffers;
};

}// namespace th
