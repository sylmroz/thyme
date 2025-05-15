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

    vk::CommandBuffer getBuffer() const {
        return m_commandBuffer;
    }

    vk::Fence getFence() const {
        return m_fence.get();
    }

    void reset();
    void start() const {
        m_commandBuffer.begin(vk::CommandBufferBeginInfo());
    }

    vk::Semaphore submit(vk::PipelineStageFlags stage);

    void waitFor(vk::UniqueSemaphore& dependSemaphore) {
        m_dependSemaphores.emplace_back(std::move(dependSemaphore));
    }

    bool isSubmitted() const {
        return m_submitted;
    }

private:
    vk::UniqueFence m_fence;
    vk::CommandBuffer m_commandBuffer;
    vk::UniqueSemaphore m_semaphore;

    vk::Device m_device;
    vk::CommandPool m_commandPool;
    vk::Queue m_graphicQueue;
    std::vector<vk::UniqueSemaphore> m_dependSemaphores;

    bool m_submitted{ true };
};

class VulkanCommandBuffersPool {
public:
    VulkanCommandBuffersPool(vk::Device device, vk::CommandPool commandPool, vk::Queue graphicQueue,
                             std::size_t capacity);
    VulkanCommandBuffer& get() {
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

    vk::Semaphore submit() {
        const auto semaphore = get().submit(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        m_current = (m_current + 1) % m_commandBuffers.size();
        return semaphore;
    }

    void flush() {
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
    std::vector<VulkanCommandBuffer> m_commandBuffers;
    std::size_t m_current{ 0 };
    vk::Device m_device;
};

}// namespace th::vulkan

#endif// VULKAN_COMMAND_BUFFERS_HPP
