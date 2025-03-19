#pragma once

#include <thyme/platform/vulkan/texture.hpp>
#include <thyme/platform/vulkan/uniform_buffer_object.hpp>
#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/renderer/structs.hpp>

#include <vulkan/vulkan.hpp>

namespace th::vulkan {

class GraphicPipeline {
public:
    GraphicPipeline() = default;

    GraphicPipeline(const GraphicPipeline&) = delete;
    GraphicPipeline(GraphicPipeline&&) = delete;

    GraphicPipeline& operator=(const GraphicPipeline&) = delete;
    GraphicPipeline& operator=(GraphicPipeline&&) = delete;

    virtual void draw(const vk::UniqueCommandBuffer& commandBuffer, const vk::Extent2D& extend) const = 0;
    virtual ~GraphicPipeline() = default;
};

class TriangleGraphicPipeline final: public GraphicPipeline {
public:
    explicit TriangleGraphicPipeline(const Device& device, const vk::UniqueRenderPass& renderPass,
                                     const vk::UniqueCommandPool& commandPool);

    inline virtual void draw(const vk::UniqueCommandBuffer& commandBuffer, const vk::Extent2D& extend) const override {
        commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
        updateUBO(extend);

        for (const auto& [buffer, descriptor] : std::views::zip(m_vertexMemoryBuffers, m_descriptorSets)) {
            commandBuffer->bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics, *m_pipelineLayout, 0, { descriptor }, {});
            commandBuffer->bindVertexBuffers(0, { *buffer.buffer }, { 0 });
            commandBuffer->bindIndexBuffer(*m_indexMemoryBuffer.buffer, 0, vk::IndexType::eUint16);
            commandBuffer->drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        }
    }

private:
    vk::UniquePipeline m_pipeline;
    vk::UniquePipelineLayout m_pipelineLayout;
    std::array<BufferMemory, 2> m_vertexMemoryBuffers;
    BufferMemory m_indexMemoryBuffer;

    vk::UniqueDescriptorSetLayout m_descriptorSetLayout;
    UniformBufferObject<renderer::MVP> m_uniformBufferObject;
    UniformBufferObject<renderer::MVP> m_uniformBufferObject2;

    vk::UniqueDescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;

    VulkanTexture m_texture;
    VulkanTexture m_texture2;

    static constexpr std::array vertices1 = {
        renderer::Vertex{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
        renderer::Vertex{ { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
        renderer::Vertex{ { 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
        renderer::Vertex{ { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } }
    };

    static constexpr std::array vertices2 = {
        renderer::Vertex{ { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
        renderer::Vertex{ { 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
        renderer::Vertex{ { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
        renderer::Vertex{ { -0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } }
    };

    static constexpr std::array<uint16_t, 6> indices = { 0, 1, 2, 2, 3, 0 };

    void updateUBO(const vk::Extent2D& extend) const;
};

}// namespace th::vulkan
