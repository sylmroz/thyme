module;

#include <filesystem>
#include <ranges>

module th.render_system.vulkan;

import th.scene.camera;

namespace th {

VulkanScenePipeline::VulkanScenePipeline(const VulkanDevice& device,
                             const vk::PipelineRenderingCreateInfo& pipelineRenderingCreateInfo,
                             std::vector<VulkanModel>& models,
                             const VulkanUniformBuffer<CameraMatrices>& cameraMatrices) {
    constexpr auto uboBinding =
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
    constexpr auto cameraUboBinding =
            vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
    constexpr auto samplerLayoutBinding = vk::DescriptorSetLayoutBinding(
            2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    constexpr auto bindings = std::array{ uboBinding, cameraUboBinding, samplerLayoutBinding };

    m_descriptorSetLayout = device.logicalDevice.createDescriptorSetLayoutUnique(
            vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateInfo{
                    .bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data() }));

    m_descriptorPool =
            createDescriptorPool(device.logicalDevice,
                                 { vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, static_cast<uint32_t>(models.size())),
                                   vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, static_cast<uint32_t>(models.size())),
                                   vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, static_cast<uint32_t>(models.size())) });

    const auto descriptorSetLayouts = std::vector{ models.size(), m_descriptorSetLayout.get() };
    m_descriptorSets = device.logicalDevice.allocateDescriptorSets(
            vk::DescriptorSetAllocateInfo{ .descriptorPool = m_descriptorPool.get(),
                                           .descriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
                                           .pSetLayouts = descriptorSetLayouts.data() });

    const auto cameraDescriptorBufferInfo = cameraMatrices.getDescriptorBufferInfos();
    for (const auto [descriptorSet, model] : std::views::zip(m_descriptorSets, models)) {
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
        device.logicalDevice.updateDescriptorSets(writeDescriptorSets, {});
    }

    const auto currentDir = std::filesystem::current_path();
    const auto shaderPath = currentDir / "../../../../thyme/shaders/spv";
    const auto shaderAbsolutePath = std::filesystem::absolute(shaderPath);
    const auto vertShader = readFile(shaderAbsolutePath / "triangle.vert.spv");
    const auto fragShader = readFile(shaderAbsolutePath / "triangle.frag.spv");
    const auto vertexShaderModule = device.logicalDevice.createShaderModuleUnique(vk::ShaderModuleCreateInfo{
            .codeSize = vertShader.size(), .pCode = reinterpret_cast<const uint32_t*>(vertShader.data()) });
    const auto fragmentShaderModule = device.logicalDevice.createShaderModuleUnique(vk::ShaderModuleCreateInfo{
            .codeSize = fragShader.size(), .pCode = reinterpret_cast<const uint32_t*>(fragShader.data()) });
    const auto vertexShaderStageInfo = vk::PipelineShaderStageCreateInfo{ .stage = vk::ShaderStageFlagBits::eVertex,
                                                                          .module = vertexShaderModule.get(),
                                                                          .pName = "main" };
    const auto fragmentShaderStageInfo = vk::PipelineShaderStageCreateInfo{ .stage = vk::ShaderStageFlagBits::eFragment,
                                                                            .module = fragmentShaderModule.get(),
                                                                            .pName = "main" };
    const auto shaderStages = std::vector{ vertexShaderStageInfo, fragmentShaderStageInfo };

    m_pipelineLayout = device.logicalDevice.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo{
            .setLayoutCount = 1,
            .pSetLayouts = &(m_descriptorSetLayout.get()),
    });
    m_pipeline = createVulkanGraphicsPipeline(device.logicalDevice,
                                        m_pipelineLayout.get(),
                                        device.maxMsaaSamples,
                                        pipelineRenderingCreateInfo,
                                        shaderStages);
}

void VulkanScenePipeline::draw(const vk::CommandBuffer commandBuffer, const std::vector<VulkanModel>& models) const {
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.get());
    for (const auto& [model, descriptor] : std::views::zip(models, m_descriptorSets)) {
        commandBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, { descriptor }, {});
        model.draw(commandBuffer);
    }
}

auto createVulkanGraphicsPipeline(const vk::Device logicalDevice,
                            vk::PipelineLayout pipelineLayout,
                            vk::SampleCountFlagBits samples,
                            vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo,
                            const std::vector<vk::PipelineShaderStageCreateInfo>& shaderStages) -> vk::UniquePipeline {
    constexpr auto dynamicStates = std::array{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    const auto dynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };
    constexpr auto bindingDescription = getBindingDescription();
    constexpr auto attributeDescriptions = getAttributeDescriptions();
    const auto vertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data(),
    };
    constexpr auto inputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo{
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = vk::False,
    };
    constexpr auto viewportState = vk::PipelineViewportStateCreateInfo{
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
    constexpr auto colorBlendAttachments = vk::PipelineColorBlendAttachmentState{
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
    const auto colorBlendStateCreateInfo =
            vk::PipelineColorBlendStateCreateInfo{ .logicOpEnable = vk::False,
                                                   .logicOp = vk::LogicOp::eCopy,
                                                   .attachmentCount = 1,
                                                   .pAttachments = &colorBlendAttachments,
                                                   .blendConstants = std::array{ 0.0f, 0.0f, 0.0f, 0.0f } };
    constexpr auto deptStencilStateCreateInfo =
            vk::PipelineDepthStencilStateCreateInfo{ .depthTestEnable = vk::True,
                                                     .depthWriteEnable = vk::True,
                                                     .depthCompareOp = vk::CompareOp::eLess,
                                                     .depthBoundsTestEnable = vk::False,
                                                     .stencilTestEnable = vk::False,
                                                     .front = {},
                                                     .back = {},
                                                     .minDepthBounds = 0.0f,
                                                     .maxDepthBounds = 1.0f };

    return logicalDevice
            .createGraphicsPipelineUnique({},
                                          vk::GraphicsPipelineCreateInfo{
                                                  .pNext = &pipelineRenderingCreateInfo,
                                                  .stageCount = static_cast<uint32_t>(shaderStages.size()),
                                                  .pStages = shaderStages.data(),
                                                  .pVertexInputState = &vertexInputStateCreateInfo,
                                                  .pInputAssemblyState = &inputAssemblyStateCreateInfo,
                                                  .pViewportState = &viewportState,
                                                  .pRasterizationState = &rasterizer,
                                                  .pMultisampleState = &multisampling,
                                                  .pDepthStencilState = &deptStencilStateCreateInfo,
                                                  .pColorBlendState = &colorBlendStateCreateInfo,
                                                  .pDynamicState = &dynamicStateCreateInfo,
                                                  .layout = pipelineLayout,
                                          })
            .value;
}

}