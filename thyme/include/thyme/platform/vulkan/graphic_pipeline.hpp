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

    virtual void draw(const vk::CommandBuffer commandBuffer) const = 0;
    virtual ~GraphicPipeline() = default;
};

class ScenePipeline final: public GraphicPipeline {
public:
    explicit ScenePipeline(const Device& device, const vk::RenderPass renderPass, const vk::CommandPool commandPool,
                           scene::ModelStorage& modelStorage, scene::Camera& camera);

    virtual void draw(const vk::CommandBuffer commandBuffer) const override;

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
