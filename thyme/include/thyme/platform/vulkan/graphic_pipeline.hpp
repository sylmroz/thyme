#pragma once

#include <thyme/platform/vulkan/model.hpp>
#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/platform/vulkan/vulkan_device.hpp>
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

    virtual void draw(vk::CommandBuffer commandBuffer) const = 0;
    virtual ~GraphicPipeline() = default;
};

class ScenePipeline final: public GraphicPipeline {
public:
    explicit ScenePipeline(const VulkanDevice& device,
                           const vk::PipelineRenderingCreateInfo& pipelineRenderingCreateInfo,
                           scene::ModelStorage& modelStorage, scene::Camera& camera);

    virtual void draw(vk::CommandBuffer commandBuffer) const override;

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
