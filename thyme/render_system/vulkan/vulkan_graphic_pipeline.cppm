export module th.render_system.vulkan:graphic_pipeline;

import std;

import vulkan;

import th.scene.camera;
import th.core.utils;
import th.core.logger;

import :device;
import :model;
import :utils;
import :uniform_buffer_object;


namespace th {

export [[nodiscard]] auto
        createVulkanGraphicsPipeline(const vk::raii::Device& logical_device,
                                     vk::PipelineLayout pipeline_layout,
                                     vk::SampleCountFlagBits samples,
                                     const vk::PipelineRenderingCreateInfo& pipeline_rendering_create_info,
                                     const std::vector<vk::PipelineShaderStageCreateInfo>& shader_stages)
                -> vk::raii::Pipeline;

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
    explicit VulkanScenePipeline(const VulkanDeviceRAII& device,
                                 const vk::PipelineRenderingCreateInfo& pipeline_rendering_create_info,
                                 std::vector<VulkanModel>& models,
                                 vk::DescriptorBufferInfo descriptor_buffer_info, Logger& logger);

    void draw(vk::CommandBuffer command_buffer, const std::vector<VulkanModel>& models) const override;

private:
    vk::raii::Pipeline m_pipeline = nullptr;
    vk::raii::PipelineLayout m_pipeline_layout = nullptr;

    vk::raii::DescriptorSetLayout m_descriptor_set_layout = nullptr;

    vk::UniqueDescriptorPool m_descriptor_pool;
    std::vector<vk::raii::DescriptorSet> m_descriptor_sets;
};

class VulkanGraphicsPipelineBuilder {
public:
private:
    vk::raii::PipelineLayout m_pipeline_layout{nullptr};
    vk::raii::DescriptorSetLayout m_descriptor_set_layout{nullptr};
    vk::raii::DescriptorPool m_descriptor_pool{nullptr};
    std::vector<vk::DescriptorSet> m_descriptor_sets;
};

}// namespace th
