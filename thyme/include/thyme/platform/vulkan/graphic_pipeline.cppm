module;

#include <vulkan/vulkan.hpp>

export module thyme.platform.vulkan:graphic_pipeline;
import :utils;

export namespace Thyme::Vulkan {

class GraphicPipeline {
public:
    GraphicPipeline() = default;

    GraphicPipeline(const GraphicPipeline&) = delete;
    GraphicPipeline(GraphicPipeline&&) = delete;

    GraphicPipeline& operator=(const GraphicPipeline&) = delete;
    GraphicPipeline& operator=(GraphicPipeline&&) = delete;

    virtual void draw(const vk::UniqueCommandBuffer& commandBuffer) const = 0;
    virtual ~GraphicPipeline() = default;
};

class TriangleGraphicPipeline final: public GraphicPipeline {
public:
    explicit TriangleGraphicPipeline(const vk::UniqueDevice& logicalDevice, const vk::UniqueRenderPass& renderPass);

    inline virtual void draw(const vk::UniqueCommandBuffer& commandBuffer) const override {
        commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
        commandBuffer->draw(3, 1, 0, 0);
    }

private:
    vk::UniquePipeline m_pipeline;
    vk::UniquePipelineLayout m_pipelineLayout;
};
}// namespace Thyme::Vulkan
