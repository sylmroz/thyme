export module th.render_system.vulkan:graphic_pipeline;

import std;

import vulkan_hpp;

import th.scene.camera;
import th.core.utils;
import th.core.logger;

import :device;
import :model;
import :utils;
import :uniform_buffer_object;


namespace th {

export [[nodiscard]] auto
        createVulkanGraphicsPipeline(vk::Device logical_device,
                                     vk::PipelineLayout pipeline_layout,
                                     vk::SampleCountFlagBits samples,
                                     const vk::PipelineRenderingCreateInfo& pipeline_rendering_create_info,
                                     const std::vector<vk::PipelineShaderStageCreateInfo>& shader_stages)
                -> vk::UniquePipeline;

export class VulkanGraphicPipeline {
public:
    VulkanGraphicPipeline() = default;

    VulkanGraphicPipeline(const VulkanGraphicPipeline&) = delete;
    VulkanGraphicPipeline(VulkanGraphicPipeline&&) = delete;

    auto operator=(const VulkanGraphicPipeline&) -> VulkanGraphicPipeline& = delete;
    auto operator=(VulkanGraphicPipeline&&) -> VulkanGraphicPipeline& = delete;

    virtual void draw(vk::CommandBuffer command_buffer, const std::vector<VulkanModel>& models) const = 0;
    virtual ~VulkanGraphicPipeline() = default;
};

export class VulkanScenePipeline final: public VulkanGraphicPipeline {
public:
    explicit VulkanScenePipeline(const VulkanDevice& device,
                                 const vk::PipelineRenderingCreateInfo& pipeline_rendering_create_info,
                                 std::vector<VulkanModel>& models,
                                 const VulkanUniformBuffer<CameraMatrices>& camera_matrices, Logger& logger);

    void draw(vk::CommandBuffer command_buffer, const std::vector<VulkanModel>& models) const override;

private:
    vk::UniquePipeline m_pipeline;
    vk::UniquePipelineLayout m_pipeline_layout;

    vk::UniqueDescriptorSetLayout m_descriptor_set_layout;

    vk::UniqueDescriptorPool m_descriptor_pool;
    std::vector<vk::DescriptorSet> m_descriptor_sets;
};

}// namespace th
