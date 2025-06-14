#ifndef VULKAN_COMMAND_BUFFERS_HPP
#define VULKAN_COMMAND_BUFFERS_HPP

#include <vulkan/vulkan.hpp>

#include <ranges>
#include <vector>

#include "thyme/core/logger.hpp"

namespace th::vulkan {

class VulkanCommandBuffer {
public:
    VulkanCommandBuffer(vk::Device device, vk::CommandPool commandPool, vk::Queue graphicQueue);

    [[nodiscard]] auto getBuffer() const -> vk::CommandBuffer{
        return m_commandBuffer;
    }

    [[nodiscard]] auto getFence() const -> vk::Fence {
        return m_fence.get();
    }

    void reset();
    void start() const {
        m_commandBuffer.begin(vk::CommandBufferBeginInfo());
    }

    void submit(vk::PipelineStageFlags stage, vk::Semaphore renderSemaphore);

    void waitFor(vk::UniqueSemaphore& dependSemaphore) {
        m_dependSemaphores.emplace_back(std::move(dependSemaphore));
    }

    auto isSubmitted() const -> bool {
        return m_submitted;
    }

private:
    bool m_submitted{ true };

    vk::UniqueFence m_fence;
    vk::CommandBuffer m_commandBuffer;
    vk::UniqueSemaphore m_semaphore;

    vk::Device m_device;
    vk::Queue m_graphicQueue;
    std::vector<vk::UniqueSemaphore> m_dependSemaphores;
};

class VulkanCommandBuffersPool {
public:
    VulkanCommandBuffersPool(vk::Device device, vk::CommandPool commandPool, vk::Queue graphicQueue,
                             std::size_t capacity);
    [[nodiscard]] auto get() -> VulkanCommandBuffer& {
        auto& currentBuffer = m_commandBuffers[m_current];
        if (currentBuffer.isSubmitted()) {
            currentBuffer.reset();
            currentBuffer.start();
        }
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

    void flush() const {
        std::vector<vk::Fence> fences;
        for (const auto& buffer : m_commandBuffers) {
            if (buffer.isSubmitted()) {
                fences.emplace_back(buffer.getFence());
            }
        }
        [[maybe_unused]] const auto result =
                m_device.waitForFences(fences, vk::True, std::numeric_limits<uint64_t>::max());
    }

private:
    std::size_t m_current{ 0 };
    vk::Device m_device;
    std::vector<VulkanCommandBuffer> m_commandBuffers;
};

}// namespace th::vulkan

#endif// VULKAN_COMMAND_BUFFERS_HPP
