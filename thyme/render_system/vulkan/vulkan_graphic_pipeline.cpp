module;

module th.render_system.vulkan;

import th.scene.camera;

namespace th {

VulkanScenePipeline::VulkanScenePipeline(const VulkanDevice& device,
                             const vk::PipelineRenderingCreateInfo& pipeline_rendering_create_info,
                             std::vector<VulkanModel>& models,
                             const VulkanUniformBuffer<CameraMatrices>& camera_matrices) {
    constexpr auto uboBinding =
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
    constexpr auto cameraUboBinding =
            vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
    constexpr auto samplerLayoutBinding = vk::DescriptorSetLayoutBinding(
            2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    constexpr auto bindings = std::array{ uboBinding, cameraUboBinding, samplerLayoutBinding };

    m_descriptor_set_layout = device.logical_device.createDescriptorSetLayoutUnique(
            vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateInfo{
                    .bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data() }));

    m_descriptor_pool =
            createDescriptorPool(device.logical_device,
                                 { vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, static_cast<uint32_t>(models.size())),
                                   vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, static_cast<uint32_t>(models.size())),
                                   vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, static_cast<uint32_t>(models.size())) });

    const auto descriptorSetLayouts = std::vector{ models.size(), m_descriptor_set_layout.get() };
    m_descriptor_sets = device.logical_device.allocateDescriptorSets(
            vk::DescriptorSetAllocateInfo{ .descriptorPool = m_descriptor_pool.get(),
                                           .descriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
                                           .pSetLayouts = descriptorSetLayouts.data() });

    const auto cameraDescriptorBufferInfo = camera_matrices.getDescriptorBufferInfos();
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
                                    .pBufferInfo = &cameraDescriptorBufferInfo,
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
        device.logical_device.updateDescriptorSets(writeDescriptorSets, {});
    }

    const auto currentDir = std::filesystem::current_path();
    const auto shaderPath = currentDir / "../../../../thyme/shaders/spv";
    const auto shaderAbsolutePath = std::filesystem::absolute(shaderPath);
    const auto vertShader = readFile(shaderAbsolutePath / "triangle.vert.spv");
    const auto fragShader = readFile(shaderAbsolutePath / "triangle.frag.spv");
    const auto vertexShaderModule = device.logical_device.createShaderModuleUnique(vk::ShaderModuleCreateInfo{
            .codeSize = vertShader.size(), .pCode = reinterpret_cast<const uint32_t*>(vertShader.data()) });
    const auto fragmentShaderModule = device.logical_device.createShaderModuleUnique(vk::ShaderModuleCreateInfo{
            .codeSize = fragShader.size(), .pCode = reinterpret_cast<const uint32_t*>(fragShader.data()) });
    const auto vertexShaderStageInfo = vk::PipelineShaderStageCreateInfo{ .stage = vk::ShaderStageFlagBits::eVertex,
                                                                          .module = vertexShaderModule.get(),
                                                                          .pName = "main" };
    const auto fragmentShaderStageInfo = vk::PipelineShaderStageCreateInfo{ .stage = vk::ShaderStageFlagBits::eFragment,
                                                                            .module = fragmentShaderModule.get(),
                                                                            .pName = "main" };
    const auto shaderStages = std::vector{ vertexShaderStageInfo, fragmentShaderStageInfo };

    m_pipeline_layout = device.logical_device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo{
            .setLayoutCount = 1,
            .pSetLayouts = &(m_descriptor_set_layout.get()),
    });
    m_pipeline = createVulkanGraphicsPipeline(device.logical_device,
                                        m_pipeline_layout.get(),
                                        device.max_msaa_samples,
                                        pipeline_rendering_create_info,
                                        shaderStages);
}

void VulkanScenePipeline::draw(const vk::CommandBuffer command_buffer, const std::vector<VulkanModel>& models) const {
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.get());
    for (const auto& [model, descriptor] : std::views::zip(models, m_descriptor_sets)) {
        command_buffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics, m_pipeline_layout.get(), 0, { descriptor }, {});
        model.draw(command_buffer);
    }
}

auto createVulkanGraphicsPipeline(const vk::Device logical_device,
                            const vk::PipelineLayout pipeline_layout,
                            const vk::SampleCountFlagBits samples,
                            const vk::PipelineRenderingCreateInfo& pipeline_rendering_create_info,
                            const std::vector<vk::PipelineShaderStageCreateInfo>& shader_stages) -> vk::UniquePipeline {
    constexpr auto dynamicStates = std::array{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    const auto dynamic_state_create_info = vk::PipelineDynamicStateCreateInfo{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
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

    return logical_device
            .createGraphicsPipelineUnique({},
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
                                          })
            .value;
}

}