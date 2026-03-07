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

class VulkanGraphicsPipelineBuilder {
public:
    [[nodiscard]] auto build(const vk::raii::Device& device, const vk::PipelineLayout& pipeline_layout,
                             const vk::Optional<const vk::raii::PipelineCache>& pipeline_cache = nullptr) const
            -> vk::raii::Pipeline;

    auto setShader(vk::PipelineShaderStageCreateInfo shader_stage_create_info) -> VulkanGraphicsPipelineBuilder&;
    auto setShaders(std::span<const vk::PipelineShaderStageCreateInfo> shader_stage_create_info_list)
            -> VulkanGraphicsPipelineBuilder&;
    auto setInputTopology(vk::PrimitiveTopology primitive_topology) -> VulkanGraphicsPipelineBuilder&;
    auto setCullMode(vk::CullModeFlags cull_mode_flags, vk::FrontFace front_face) -> VulkanGraphicsPipelineBuilder&;
    auto setMultisampling(vk::SampleCountFlagBits samples) -> VulkanGraphicsPipelineBuilder&;
    auto enableBlending(vk::PipelineColorBlendAttachmentState state) -> VulkanGraphicsPipelineBuilder&;
    auto setColorAttachmentFormats(std::span<const vk::Format> formats) -> VulkanGraphicsPipelineBuilder&;
    auto setDepthAttachmentFormat(vk::Format format) -> VulkanGraphicsPipelineBuilder&;
    auto disableDepthTest() -> VulkanGraphicsPipelineBuilder&;
    auto enableDepthStencil(const vk::PipelineDepthStencilStateCreateInfo& info) -> VulkanGraphicsPipelineBuilder&;
    auto setVertexInputState(const vk::PipelineVertexInputStateCreateInfo& info) -> VulkanGraphicsPipelineBuilder&;

private:
    std::vector<vk::Format> m_color_attachment_formats{ vk::Format::eA8B8G8R8UnormPack32 };
    std::vector<vk::PipelineShaderStageCreateInfo> m_shader_stages;

    vk::PipelineRenderingCreateInfo m_rendering_create_info;
    vk::PipelineColorBlendAttachmentState m_blend_attachment_state;
    vk::PipelineVertexInputStateCreateInfo m_vertex_input_state_create_info;
    vk::PipelineInputAssemblyStateCreateInfo m_input_assembly_state_create_info;
    // vk::PipelineTessellationStateCreateInfo m_tessellation_state_create_info;
    vk::PipelineRasterizationStateCreateInfo m_rasterization_state_create_info;
    vk::PipelineMultisampleStateCreateInfo m_multisample_state_create_info;
    vk::PipelineDepthStencilStateCreateInfo m_depth_stencil_state_create_info;
    // vk::PipelineDynamicStateCreateInfo m_dynamic_state_create_info;
};

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
    explicit VulkanScenePipeline(const vk::raii::Device& device, vk::SampleCountFlagBits msaa_samples,
                                 const vk::PipelineRenderingCreateInfo& pipeline_rendering_create_info,
                                 std::vector<VulkanModel>& models,
                                 const vk::DescriptorBufferInfo& descriptor_buffer_info, Logger& logger);

    void draw(vk::CommandBuffer command_buffer, const std::vector<VulkanModel>& models) const override;

private:
    vk::raii::Pipeline m_pipeline = nullptr;
    vk::raii::PipelineLayout m_pipeline_layout = nullptr;

    vk::raii::DescriptorSetLayout m_descriptor_set_layout = nullptr;

    vk::raii::DescriptorPool m_descriptor_pool = nullptr;
    std::vector<vk::raii::DescriptorSet> m_descriptor_sets;
};

class DescriptorLayoutBuilder {
public:
    void addBinding(const uint32_t binding, const vk::DescriptorType type, const vk::ShaderStageFlagBits stage) {
        addBinding(vk::DescriptorSetLayoutBinding{
                .binding = binding,
                .descriptorType = type,
                .descriptorCount = 1,
                .stageFlags = stage,
        });
    }

    void addBinding(vk::DescriptorSetLayoutBinding binding) {
        m_descriptor_sets_layout_bindings.emplace_back(binding);
        m_descriptor_types_ratio.contains(binding.descriptorType)
                ? m_descriptor_types_ratio[binding.descriptorType]++
                : m_descriptor_types_ratio[binding.descriptorType] = 1;
    }

    [[nodiscard]] auto getDescriptorTypesRatio() const -> std::vector<std::pair<vk::DescriptorType, uint32_t>>;

    [[nodiscard]] auto build(const vk::raii::Device& device) const -> vk::raii::DescriptorSetLayout;

private:
    std::vector<vk::DescriptorSetLayoutBinding> m_descriptor_sets_layout_bindings;
    std::map<vk::DescriptorType, uint32_t> m_descriptor_types_ratio;
};

class GradientPipeline {
public:
    GradientPipeline(const vk::raii::Device& device, vk::DescriptorPool descriptor_pool, vk::ImageView image_view,
                     Logger& logger);
    void dispatch(vk::CommandBuffer command_buffer) const;

private:
    vk::raii::Pipeline m_pipeline{ nullptr };
    vk::raii::PipelineLayout m_pipeline_layout{ nullptr };
    vk::raii::DescriptorSetLayout m_descriptor_set_layout{ nullptr };
    vk::raii::DescriptorSet m_descriptor_set{ nullptr };
};

}// namespace th
