module;

module th.render_system.vulkan;

import th.scene.camera;

namespace th {

VulkanScenePipeline::VulkanScenePipeline(const vk::raii::Device& device, vk::SampleCountFlagBits msaa_samples,
                                         const vk::PipelineRenderingCreateInfo& pipeline_rendering_create_info,
                                         std::vector<VulkanModel>& models,
                                         const vk::DescriptorBufferInfo& descriptor_buffer_info, Logger& logger) {

    DescriptorLayoutBuilder layout_builder;
    layout_builder.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
    layout_builder.addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
    layout_builder.addBinding(2, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
    m_descriptor_set_layout = layout_builder.build(device);

    m_pipeline_layout = device.createPipelineLayout(vk::PipelineLayoutCreateInfo{
            .setLayoutCount = 1,
            .pSetLayouts = &(*m_descriptor_set_layout),
    });

    const auto descriptor_types_ratio = layout_builder.getDescriptorTypesRatio();
    m_descriptor_pool = createDescriptorPool(device, descriptor_types_ratio, static_cast<uint32_t>(models.size()));

    const auto descriptorSetLayouts = std::vector{ models.size(), *m_descriptor_set_layout };
    m_descriptor_sets = device.allocateDescriptorSets(
            vk::DescriptorSetAllocateInfo{ .descriptorPool = m_descriptor_pool,
                                           .descriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
                                           .pSetLayouts = descriptorSetLayouts.data() });

    for (const auto [descriptorSet, model] : std::views::zip(m_descriptor_sets, models)) {
        const auto descriptorBufferInfo = model.getUniformBufferObject().getDescriptorBufferInfos();
        const auto descriptorImageInfo =
                model.getTexture().getDescriptorImageInfo(vk::ImageLayout::eShaderReadOnlyOptimal);
        const auto writeDescriptorSets =
                std::array{ vk::WriteDescriptorSet{
                                    .dstSet = descriptorSet,
                                    .dstBinding = 0u,
                                    .dstArrayElement = 0u,
                                    .descriptorCount = 1u,
                                    .descriptorType = vk::DescriptorType::eUniformBuffer,
                                    .pBufferInfo = &descriptor_buffer_info,
                            },
                            vk::WriteDescriptorSet{
                                    .dstSet = descriptorSet,
                                    .dstBinding = 1u,
                                    .dstArrayElement = 0u,
                                    .descriptorCount = 1u,
                                    .descriptorType = vk::DescriptorType::eUniformBuffer,
                                    .pBufferInfo = &descriptorBufferInfo,
                            },
                            vk::WriteDescriptorSet{
                                    .dstSet = descriptorSet,
                                    .dstBinding = 2u,
                                    .dstArrayElement = 0u,
                                    .descriptorCount = 1u,
                                    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                    .pImageInfo = &descriptorImageInfo,
                            } };
        device.updateDescriptorSets(writeDescriptorSets, {});
    }

    const auto slang_shader = compileSlangShader("triangle");
    auto shader_module = createShaderModule(device, std::span{ slang_shader }, logger);
    auto vertex_shader_stage_info = vk::PipelineShaderStageCreateInfo{ .stage = vk::ShaderStageFlagBits::eVertex,
                                                                       .module = shader_module,
                                                                       .pName = "main" };

    auto frag_shader_stage_info = vk::PipelineShaderStageCreateInfo{ .stage = vk::ShaderStageFlagBits::eFragment,
                                                                     .module = shader_module,
                                                                     .pName = "main" };

    const auto shader_stages = std::vector{ vertex_shader_stage_info, frag_shader_stage_info };

    const auto formats = std::array{ *pipeline_rendering_create_info.pColorAttachmentFormats };

    constexpr auto binding_description = getBindingDescription();
    constexpr auto attribute_descriptions = getAttributeDescriptions();
    const auto vertex_input_state_create_info = vk::PipelineVertexInputStateCreateInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding_description,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size()),
        .pVertexAttributeDescriptions = attribute_descriptions.data(),
    };

    m_pipeline =
            VulkanGraphicsPipelineBuilder{}
                    .setMultisampling(msaa_samples)
                    .setColorAttachmentFormats(formats)
                    .setDepthAttachmentFormat(pipeline_rendering_create_info.depthAttachmentFormat)
                    .enableBlending(vk::PipelineColorBlendAttachmentState{
                            .blendEnable = vk::False,
                            .srcColorBlendFactor = vk::BlendFactor::eOne,
                            .dstColorBlendFactor = vk::BlendFactor::eZero,
                            .colorBlendOp = vk::BlendOp::eAdd,
                            .srcAlphaBlendFactor = vk::BlendFactor::eOne,
                            .dstAlphaBlendFactor = vk::BlendFactor::eZero,
                            .alphaBlendOp = vk::BlendOp::eAdd,
                            .colorWriteMask = vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR
                                              | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB })
                    .enableDepthStencil(vk::PipelineDepthStencilStateCreateInfo{ .depthTestEnable = vk::True,
                                                                                 .depthWriteEnable = vk::True,
                                                                                 .depthCompareOp = vk::CompareOp::eLess,
                                                                                 .depthBoundsTestEnable = vk::False,
                                                                                 .stencilTestEnable = vk::False,
                                                                                 .front = {},
                                                                                 .back = {},
                                                                                 .minDepthBounds = 0.0f,
                                                                                 .maxDepthBounds = 1.0f })
                    .setCullMode(vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise)
                    .setShaders(shader_stages)
                    .setInputTopology(vk::PrimitiveTopology::eTriangleList)
                    .setVertexInputState(vertex_input_state_create_info)
                    .build(device, *m_pipeline_layout);

    /*m_pipeline = createVulkanGraphicsPipeline(device.logical_device,
                                              *m_pipeline_layout,
                                              device.max_msaa_samples,
                                              pipeline_rendering_create_info,
                                              shader_stages);*/
}

void VulkanScenePipeline::draw(const vk::CommandBuffer command_buffer, const std::vector<VulkanModel>& models) const {
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
    for (const auto& [model, descriptor] : std::views::zip(models, m_descriptor_sets)) {
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipeline_layout, 0, { descriptor }, {});
        model.draw(command_buffer);
    }
}

[[nodiscard]] auto DescriptorLayoutBuilder::getDescriptorTypesRatio() const
        -> std::vector<std::pair<vk::DescriptorType, uint32_t>> {
    return m_descriptor_types_ratio | std::views::transform([](auto pair) {
               return pair;
           })
           | std::ranges::to<std::vector<std::pair<vk::DescriptorType, uint32_t>>>();
}

[[nodiscard]] auto DescriptorLayoutBuilder::build(const vk::raii::Device& device) const
        -> vk::raii::DescriptorSetLayout {
    return device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{
            .bindingCount = static_cast<uint32_t>(m_descriptor_sets_layout_bindings.size()),
            .pBindings = m_descriptor_sets_layout_bindings.data() });
}

GradientPipeline::GradientPipeline(const vk::raii::Device& device, const vk::DescriptorPool descriptor_pool,
                                   const vk::ImageView image_view, Logger& logger) {
    logger.info("Create gradient pipeline");
    {
        DescriptorLayoutBuilder descriptor_layout_builder;
        descriptor_layout_builder.addBinding(0, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute);
        m_descriptor_set_layout = descriptor_layout_builder.build(device);
    }
    m_descriptor_set = std::move(
            device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo{ .descriptorPool = descriptor_pool,
                                                                         .descriptorSetCount = 1,
                                                                         .pSetLayouts = &(*m_descriptor_set_layout) })
                    .front());

    const auto descriptor_image_info =
            vk::DescriptorImageInfo{ .imageView = image_view, .imageLayout = vk::ImageLayout::eGeneral };
    const auto write_descriptor_sets =
            std::array{ vk::WriteDescriptorSet{ .dstSet = m_descriptor_set,
                                                .descriptorCount = 1,
                                                .descriptorType = vk::DescriptorType::eStorageImage,
                                                .pImageInfo = &descriptor_image_info } };
    device.updateDescriptorSets(write_descriptor_sets, {});

    m_pipeline_layout = device.createPipelineLayout(
            vk::PipelineLayoutCreateInfo{ .setLayoutCount = 1, .pSetLayouts = &(*m_descriptor_set_layout) });

    const auto compute_shader_code = compileSlangShader("compute");
    const auto compute_shader_module = createShaderModule(device, std::span{ compute_shader_code }, logger);
    const auto compute_shader_stage_info = vk::PipelineShaderStageCreateInfo{
        .stage = vk::ShaderStageFlagBits::eCompute, .module = compute_shader_module, .pName = "main"
    };
    m_pipeline = device.createComputePipeline(
            nullptr, vk::ComputePipelineCreateInfo{ .stage = compute_shader_stage_info, .layout = m_pipeline_layout });
}

void GradientPipeline::dispatch(const vk::CommandBuffer command_buffer) const {
    command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, m_pipeline);
    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_pipeline_layout, 0, { *m_descriptor_set }, {});
}

auto VulkanGraphicsPipelineBuilder::build(const vk::raii::Device& device, const vk::PipelineLayout& pipeline_layout,
                                          const vk::Optional<const vk::raii::PipelineCache>& pipeline_cache) const
        -> vk::raii::Pipeline {
    constexpr auto viewport_state = vk::PipelineViewportStateCreateInfo{
        .viewportCount = 1,
        .scissorCount = 1,
    };

    const auto color_blend_state_create_info =
            vk::PipelineColorBlendStateCreateInfo{ .logicOpEnable = vk::False,
                                                   .logicOp = vk::LogicOp::eCopy,
                                                   .attachmentCount = 1,
                                                   .pAttachments = &m_blend_attachment_state,
                                                   .blendConstants = std::array{ 0.0f, 0.0f, 0.0f, 0.0f } };
    constexpr auto dynamic_states = std::array{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    const auto dynamic_state_create_info = vk::PipelineDynamicStateCreateInfo{
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data(),
    };

    return device.createGraphicsPipeline(pipeline_cache,
                                         vk::GraphicsPipelineCreateInfo{
                                                 .pNext = &m_rendering_create_info,
                                                 .stageCount = static_cast<uint32_t>(m_shader_stages.size()),
                                                 .pStages = m_shader_stages.data(),
                                                 .pVertexInputState = &m_vertex_input_state_create_info,
                                                 .pInputAssemblyState = &m_input_assembly_state_create_info,
                                                 .pTessellationState = nullptr,
                                                 .pViewportState = &viewport_state,
                                                 .pRasterizationState = &m_rasterization_state_create_info,
                                                 .pMultisampleState = &m_multisample_state_create_info,
                                                 .pDepthStencilState = &m_depth_stencil_state_create_info,
                                                 .pColorBlendState = &color_blend_state_create_info,
                                                 .pDynamicState = &dynamic_state_create_info,
                                                 .layout = pipeline_layout,
                                         });
}

auto VulkanGraphicsPipelineBuilder::setShader(vk::PipelineShaderStageCreateInfo shader_stage_create_info)
        -> VulkanGraphicsPipelineBuilder& {
    m_shader_stages.emplace_back(shader_stage_create_info);
    return *this;
}

auto VulkanGraphicsPipelineBuilder::setShaders(
        std::span<const vk::PipelineShaderStageCreateInfo> shader_stage_create_info_list)
        -> VulkanGraphicsPipelineBuilder& {
    m_shader_stages.clear();
    std::ranges::copy(shader_stage_create_info_list, std::back_inserter(m_shader_stages));
    return *this;
}

auto VulkanGraphicsPipelineBuilder::setInputTopology(const vk::PrimitiveTopology primitive_topology)
        -> VulkanGraphicsPipelineBuilder& {
    m_input_assembly_state_create_info.primitiveRestartEnable = vk::False;
    m_input_assembly_state_create_info.topology = primitive_topology;
    return *this;
}

auto VulkanGraphicsPipelineBuilder::setCullMode(const vk::CullModeFlags cull_mode_flags, const vk::FrontFace front_face)
        -> VulkanGraphicsPipelineBuilder& {
    m_rasterization_state_create_info = vk::PipelineRasterizationStateCreateInfo{ .depthClampEnable = vk::False,
                                                                                  .rasterizerDiscardEnable = vk::False,
                                                                                  .polygonMode = vk::PolygonMode::eFill,
                                                                                  .cullMode = cull_mode_flags,
                                                                                  .frontFace = front_face,
                                                                                  .depthBiasEnable = vk::False,
                                                                                  .depthBiasConstantFactor = 0.0f,
                                                                                  .depthBiasClamp = 0.0f,
                                                                                  .depthBiasSlopeFactor = 0.0f,
                                                                                  .lineWidth = 1.0f };
    return *this;
}

auto VulkanGraphicsPipelineBuilder::setMultisampling(const vk::SampleCountFlagBits samples)
        -> VulkanGraphicsPipelineBuilder& {
    m_multisample_state_create_info = vk::PipelineMultisampleStateCreateInfo{
        .rasterizationSamples = samples,
        .sampleShadingEnable = vk::False,
        .minSampleShading = 1.0f,
        .alphaToCoverageEnable = vk::False,
        .alphaToOneEnable = vk::False,
    };
    return *this;
}
auto VulkanGraphicsPipelineBuilder::enableBlending(vk::PipelineColorBlendAttachmentState state)
        -> VulkanGraphicsPipelineBuilder& {
    m_blend_attachment_state = state;
    return *this;
}

auto VulkanGraphicsPipelineBuilder::setColorAttachmentFormats(std::span<const vk::Format> formats)
        -> VulkanGraphicsPipelineBuilder& {
    m_color_attachment_formats.clear();
    std::ranges::copy(formats, std::back_inserter(m_color_attachment_formats));
    m_rendering_create_info.colorAttachmentCount = static_cast<uint32_t>(m_color_attachment_formats.size());
    m_rendering_create_info.pColorAttachmentFormats = m_color_attachment_formats.data();
    return *this;
}

auto VulkanGraphicsPipelineBuilder::setDepthAttachmentFormat(const vk::Format format)
        -> VulkanGraphicsPipelineBuilder& {
    m_rendering_create_info.depthAttachmentFormat = format;
    return *this;
}

auto VulkanGraphicsPipelineBuilder::disableDepthTest() -> VulkanGraphicsPipelineBuilder& {
    m_depth_stencil_state_create_info = vk::PipelineDepthStencilStateCreateInfo{
        .depthTestEnable = vk::False,
        .depthWriteEnable = vk::False,
        .depthCompareOp = vk::CompareOp::eNever,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable = vk::False,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };
    return *this;
}

auto VulkanGraphicsPipelineBuilder::enableDepthStencil(const vk::PipelineDepthStencilStateCreateInfo& info)
        -> VulkanGraphicsPipelineBuilder& {
    m_depth_stencil_state_create_info = info;
    return *this;
}

auto VulkanGraphicsPipelineBuilder::setVertexInputState(const vk::PipelineVertexInputStateCreateInfo& info)
        -> VulkanGraphicsPipelineBuilder& {
    m_vertex_input_state_create_info = info;
    return *this;
}

auto createVulkanGraphicsPipeline(const vk::raii::Device& logical_device,
                                  const vk::PipelineLayout pipeline_layout,
                                  const vk::SampleCountFlagBits samples,
                                  const vk::PipelineRenderingCreateInfo& pipeline_rendering_create_info,
                                  const std::vector<vk::PipelineShaderStageCreateInfo>& shader_stages)
        -> vk::raii::Pipeline {
    constexpr auto dynamic_states = std::array{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    const auto dynamic_state_create_info = vk::PipelineDynamicStateCreateInfo{
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data(),
    };
    constexpr auto binding_description = getBindingDescription();
    constexpr auto attribute_descriptions = getAttributeDescriptions();
    const auto vertex_input_state_create_info = vk::PipelineVertexInputStateCreateInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding_description,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size()),
        .pVertexAttributeDescriptions = attribute_descriptions.data(),
    };
    constexpr auto input_assembly_state_create_info = vk::PipelineInputAssemblyStateCreateInfo{
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = vk::False,
    };
    constexpr auto viewport_state = vk::PipelineViewportStateCreateInfo{
        .viewportCount = 1,
        .scissorCount = 1,
    };
    constexpr auto rasterizer = vk::PipelineRasterizationStateCreateInfo{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };
    const auto multisampling = vk::PipelineMultisampleStateCreateInfo{
        .rasterizationSamples = samples,
        .sampleShadingEnable = vk::False,
        .minSampleShading = 1.0f,
        .alphaToCoverageEnable = vk::False,
        .alphaToOneEnable = vk::False,
    };
    constexpr auto color_blend_attachments = vk::PipelineColorBlendAttachmentState{
        .blendEnable = vk::False,
        .srcColorBlendFactor = vk::BlendFactor::eOne,
        .dstColorBlendFactor = vk::BlendFactor::eZero,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR
                          | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
    };
    const auto color_blend_state_create_info =
            vk::PipelineColorBlendStateCreateInfo{ .logicOpEnable = vk::False,
                                                   .logicOp = vk::LogicOp::eCopy,
                                                   .attachmentCount = 1,
                                                   .pAttachments = &color_blend_attachments,
                                                   .blendConstants = std::array{ 0.0f, 0.0f, 0.0f, 0.0f } };
    constexpr auto dept_stencil_state_create_info =
            vk::PipelineDepthStencilStateCreateInfo{ .depthTestEnable = vk::True,
                                                     .depthWriteEnable = vk::True,
                                                     .depthCompareOp = vk::CompareOp::eLess,
                                                     .depthBoundsTestEnable = vk::False,
                                                     .stencilTestEnable = vk::False,
                                                     .front = {},
                                                     .back = {},
                                                     .minDepthBounds = 0.0f,
                                                     .maxDepthBounds = 1.0f };

    return logical_device.createGraphicsPipeline(nullptr,
                                                 vk::GraphicsPipelineCreateInfo{
                                                         .pNext = &pipeline_rendering_create_info,
                                                         .stageCount = static_cast<uint32_t>(shader_stages.size()),
                                                         .pStages = shader_stages.data(),
                                                         .pVertexInputState = &vertex_input_state_create_info,
                                                         .pInputAssemblyState = &input_assembly_state_create_info,
                                                         .pViewportState = &viewport_state,
                                                         .pRasterizationState = &rasterizer,
                                                         .pMultisampleState = &multisampling,
                                                         .pDepthStencilState = &dept_stencil_state_create_info,
                                                         .pColorBlendState = &color_blend_state_create_info,
                                                         .pDynamicState = &dynamic_state_create_info,
                                                         .layout = pipeline_layout,
                                                 });
}

}// namespace th