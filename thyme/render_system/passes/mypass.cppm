export module th.render_system.passes:mypass;

import std;
import vulkan;

import th.core.logger;
import th.render_system.render_graph;
import th.render_system.vulkan;

namespace th {

export struct PassDrawContext {
    vk::CommandBuffer command_buffer;
    uint32_t frame_index;
};

export class MyPass {
public:
    MyPass(vk::raii::PhysicalDevice& physical_device, const vk::raii::Device& device, const vk::Format format,
           std::span<const vk::DescriptorBufferInfo> camera_descriptor_buffer_info, const Logger& logger) {
        const auto slang_shader = compileSlangShader("triangle2");
        const auto shader_module = createShaderModule(device, std::span{ slang_shader }, logger);
        const auto vertex_shader_stage_info = vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eVertex, .module = shader_module, .pName = "main"
        };

        const auto frag_shader_stage_info = vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eFragment, .module = shader_module, .pName = "main"
        };

        constexpr auto ubo_binding =
                vk::DescriptorSetLayoutBinding{ .binding = 0,
                                                .descriptorType = vk::DescriptorType::eUniformBuffer,
                                                .descriptorCount = 1,
                                                .stageFlags = vk::ShaderStageFlagBits::eVertex };
        const auto layout_info = vk::DescriptorSetLayoutCreateInfo{ .bindingCount = 1, .pBindings = &ubo_binding };

        m_descriptor_set_layout = device.createDescriptorSetLayout(layout_info);


        m_pipeline_layout = device.createPipelineLayout(
                vk::PipelineLayoutCreateInfo{ .setLayoutCount = 1, .pSetLayouts = &*m_descriptor_set_layout });


        const auto color_formats = std::array{ format };
        const auto pipeline_rendering_create_info = vk::PipelineRenderingCreateInfo{
            .colorAttachmentCount = static_cast<std::uint32_t>(color_formats.size()),
            .pColorAttachmentFormats = color_formats.data(),
        };
        const auto shader_stages = std::vector{ vertex_shader_stage_info, frag_shader_stage_info };
        m_pipeline =
                VulkanGraphicsPipelineBuilder{}
                        .setMultisampling(vk::SampleCountFlagBits::e1)
                        .setColorAttachmentFormats(color_formats)
                        .disableDepthTest()
                        // .setDepthAttachmentFormat(pipeline_rendering_create_info.depthAttachmentFormat)
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
                        .disableDepthTest()
                        /*.enableDepthStencil(
                                vk::PipelineDepthStencilStateCreateInfo{ .depthTestEnable = vk::True,
                                                                         .depthWriteEnable = vk::True,
                                                                         .depthCompareOp = vk::CompareOp::eLess,
                                                                         .depthBoundsTestEnable = vk::False,
                                                                         .stencilTestEnable = vk::False,
                                                                         .front = {},
                                                                         .back = {},
                                                                         .minDepthBounds = 0.0f,
                                                                         .maxDepthBounds = 1.0f })*/
                        .setCullMode(vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise)
                        .setShaders(shader_stages)
                        .setInputTopology(vk::PrimitiveTopology::eTriangleList)
                        // .setVertexInputState(vertex_input_state_create_info)
                        .build(device, *m_pipeline_layout);

        // descriptor sets
        const auto pool_size = vk::DescriptorPoolSize{ .type = vk::DescriptorType::eUniformBuffer,
                                                       .descriptorCount = static_cast<uint32_t>(
                                                               camera_descriptor_buffer_info.size()) };
        m_descriptor_pool = device.createDescriptorPool(
                vk::DescriptorPoolCreateInfo{ .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                                              .maxSets = static_cast<uint32_t>(camera_descriptor_buffer_info.size()),
                                              .poolSizeCount = 1,
                                              .pPoolSizes = &pool_size });

        const auto layouts = std::vector(camera_descriptor_buffer_info.size(), *m_descriptor_set_layout);
        m_descriptor_sets = device.allocateDescriptorSets(
                vk::DescriptorSetAllocateInfo{ .descriptorPool = m_descriptor_pool,
                                               .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
                                               .pSetLayouts = layouts.data() });
        for (const auto& [buffer_info, descriptor_set] :
             std::views::zip(camera_descriptor_buffer_info, m_descriptor_sets)) {

            const auto descriptor_write = vk::WriteDescriptorSet{
                .dstSet = descriptor_set,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo = &buffer_info,
            };
            device.updateDescriptorSets({ descriptor_write }, {});
        }
    }

    void draw(const PassDrawContext pass_draw_context) const {
        pass_draw_context.command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
        pass_draw_context.command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                                            *m_pipeline_layout,
                                                            0,
                                                            *m_descriptor_sets[pass_draw_context.frame_index],
                                                            nullptr);
        pass_draw_context.command_buffer.draw(3, 1, 0, 0);
    }

    void setup(RenderGraph& render_graph, const RenderGraphResource resource) const {
        render_graph.addPass("triangle2", [&, resource](RenderGraphBuilder& builder) -> execute_function {
            builder.write(resource,
                          ImageTransition{
                                  .layout = vk::ImageLayout::eColorAttachmentOptimal,
                                  .pipeline_stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                  .access_flag_bits = vk::AccessFlagBits2::eColorAttachmentWrite,
                          });

            return [=](const RenderGraphContext& context, const vk::CommandBuffer command_buffer) -> void {
                const auto texture = std::get<RenderGraphPersistentTarget>(context.targets[resource.id]);
                setCommandBufferFrameSize(command_buffer, texture.target.getResolution());
                constexpr auto clear_color_values = vk::ClearValue(vk::ClearColorValue(1.0f, 0.0f, 1.0f, 1.0f));
                const auto color_attachment = vk::RenderingAttachmentInfo{
                    .imageView = texture.target.getImageView(),
                    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                    .resolveMode = vk::ResolveModeFlagBits::eNone,
                    /*.resolveImageView = m_resolve_color_image_memory.getImageView(),
                    .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,*/
                    .loadOp = vk::AttachmentLoadOp::eClear,
                    .storeOp = vk::AttachmentStoreOp::eStore,
                    .clearValue = clear_color_values,
                };
                const auto rendering_info = vk::RenderingInfo{
                    .renderArea = vk::Rect2D{ .offset = vk::Offset2D{ .x = 0, .y = 0 },
                                              .extent = texture.target.getResolution() },
                    .layerCount = 1,
                    .viewMask = 0,
                    .colorAttachmentCount = 1,
                    .pColorAttachments = &color_attachment,
                };
                command_buffer.beginRendering(rendering_info);

                draw(PassDrawContext{ .command_buffer = command_buffer, .frame_index = context.frame_index });

                command_buffer.endRendering();
            };
        });
    }

private:
    vk::raii::PipelineLayout m_pipeline_layout = nullptr;
    vk::raii::Pipeline m_pipeline = nullptr;
    vk::raii::DescriptorSetLayout m_descriptor_set_layout = nullptr;
    vk::raii::DescriptorPool m_descriptor_pool = nullptr;
    std::vector<vk::raii::DescriptorSet> m_descriptor_sets;
};

}// namespace th
