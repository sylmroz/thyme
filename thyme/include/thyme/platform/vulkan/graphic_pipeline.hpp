#pragma once

#include <thyme/platform/vulkan/model.hpp>
#include <thyme/platform/vulkan/texture.hpp>
#include <thyme/platform/vulkan/uniform_buffer_object.hpp>
#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/renderer/structs.hpp>
#include <thyme/scene/camera.hpp>

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
                                     const vk::UniqueCommandPool& commandPool, scene::ModelStorage& modelStorage,
                                     scene::Camera& camera);

    virtual void draw(const vk::UniqueCommandBuffer& commandBuffer, const vk::Extent2D& extend) const override {
        updateUBO();

        commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
        for (const auto& [model, descriptor] : std::views::zip(m_models, m_descriptorSets)) {
            commandBuffer->bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, { descriptor }, {});
            commandBuffer->bindVertexBuffers(0, { model.getVertexMemoryBuffer().getBuffer().get() }, { 0 });
            commandBuffer->bindIndexBuffer(model.getIndexMemoryBuffer().getBuffer().get(), 0, vk::IndexType::eUint32);
            commandBuffer->drawIndexed(model.getIndicesSize(), 1, 0, 0, 0);
        }
    }

private:
    vk::UniquePipeline m_pipeline;
    vk::UniquePipelineLayout m_pipelineLayout;

    std::vector<VulkanModel> m_models;
    scene::Camera& m_camera;

    vk::UniqueDescriptorSetLayout m_descriptorSetLayout;

    vk::UniqueDescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;

    void updateUBO() const;
};

}// namespace th::vulkan
