module;

#include <vector>

export module th.render_system.vulkan:graphic_pipeline;

import vulkan_hpp;

import th.core.utils;

import :device;
import :model;
import :utils;
import :uniform_buffer_object;

import th.scene.camera;

import th.core.utils;

namespace th {

export [[nodiscard]] auto
        createVulkanGraphicsPipeline(vk::Device logicalDevice,
                                     vk::PipelineLayout pipelineLayout,
                                     vk::SampleCountFlagBits samples,
                                     vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo,
                                     const std::vector<vk::PipelineShaderStageCreateInfo>& shaderStages)
                -> vk::UniquePipeline;

export class VulkanGraphicPipeline {
public:
    VulkanGraphicPipeline() = default;

    VulkanGraphicPipeline(const VulkanGraphicPipeline&) = delete;
    VulkanGraphicPipeline(VulkanGraphicPipeline&&) = delete;

    auto operator=(const VulkanGraphicPipeline&) -> VulkanGraphicPipeline& = delete;
    auto operator=(VulkanGraphicPipeline&&) -> VulkanGraphicPipeline& = delete;

    virtual void draw(vk::CommandBuffer commandBuffer, const std::vector<VulkanModel>& models) const = 0;
    virtual ~VulkanGraphicPipeline() = default;
};

export class VulkanScenePipeline final: public VulkanGraphicPipeline {
public:
    explicit VulkanScenePipeline(const VulkanDevice& device,
                                 const vk::PipelineRenderingCreateInfo& pipelineRenderingCreateInfo,
                                 std::vector<VulkanModel>& models,
                                 const VulkanUniformBuffer<CameraMatrices>& cameraMatrices);

    void draw(vk::CommandBuffer commandBuffer, const std::vector<VulkanModel>& models) const override;

private:
    vk::UniquePipeline m_pipeline;
    vk::UniquePipelineLayout m_pipelineLayout;

    vk::UniqueDescriptorSetLayout m_descriptorSetLayout;

    vk::UniqueDescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;
};

}// namespace th
