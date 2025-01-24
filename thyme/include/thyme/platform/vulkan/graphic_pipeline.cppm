module;

#include <vulkan/vulkan.hpp>

export module thyme.platform.vulkan:graphic_pipeline;
import :utils;

namespace Thyme::Vulkan {

export class GraphicPipeline {
public:
    GraphicPipeline() = default;

    GraphicPipeline(const GraphicPipeline&) = delete;
    GraphicPipeline(GraphicPipeline&&) = delete;

    GraphicPipeline& operator=(const GraphicPipeline&) = delete;
    GraphicPipeline& operator=(GraphicPipeline&&) = delete;

    virtual void draw(const vk::UniqueCommandBuffer& commandBuffer, const vk::Extent2D& extend, const uint32_t currentImage) const = 0;
    virtual ~GraphicPipeline() = default;
};

export class TriangleGraphicPipeline final: public GraphicPipeline {
public:
    explicit TriangleGraphicPipeline(const Device& device, const vk::UniqueRenderPass& renderPass,
                                     const vk::UniqueCommandPool& commandPool, const vk::UniqueSampler& sampler);

    inline virtual void draw(const vk::UniqueCommandBuffer& commandBuffer, const vk::Extent2D& extend, const uint32_t currentImage) const override {
        updateUBO(currentImage, extend);
        commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
        commandBuffer->bindVertexBuffers(0, { *m_vertexMemoryBuffer.buffer }, { 0 });
        commandBuffer->bindIndexBuffer(*m_indexMemoryBuffer.buffer, 0, vk::IndexType::eUint16);
        commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipelineLayout, 0, {m_descriptorSets[currentImage]}, {});
        commandBuffer->drawIndexed(indices.size(), 1, 0, 0, 0);
    }
private:
    vk::UniquePipeline m_pipeline;
    vk::UniquePipelineLayout m_pipelineLayout;
    BufferMemory m_vertexMemoryBuffer;
    BufferMemory m_indexMemoryBuffer;

    vk::UniqueDescriptorSetLayout m_descriptorSetLayout;
    // 2 = max frames in flight
    std::array<BufferMemory, 2> m_uniformMemoryBuffer;
    std::array<void*, 2> m_mappedMemoryBuffer;

    vk::UniqueDescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;

    ImageMemory m_imageMemory;

    vk::UniqueSampler m_sampler;

    static constexpr std::array vertices = { Vertex{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, {1.0f, 0.0f} },
                                             Vertex{ { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, {0.0f, 0.0f } },
                                             Vertex{ { 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, {0.0f, 1.0f } },
                                             Vertex{ { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {1.0f, 1.0f} } };
    static constexpr std::array<uint16_t, 6> indices = { 0, 1, 2, 2, 3, 0 };

    void updateUBO(const uint32_t currentImage, const vk::Extent2D& extend) const;
};
}// namespace Thyme::Vulkan
