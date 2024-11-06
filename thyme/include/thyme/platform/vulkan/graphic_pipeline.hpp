#pragma once

#include <vulkan/vulkan.hpp>

#include <filesystem>

//export module thyme.platform.vulkan:graphic_pipeline;
import thyme.core.utils;

namespace Thyme::Vulkan {

class GraphicPipeline {
public:
    GraphicPipeline() = default;

    GraphicPipeline(const GraphicPipeline&) = delete;
    GraphicPipeline(GraphicPipeline&&) = delete;

    GraphicPipeline& operator=(const GraphicPipeline&) = delete;
    GraphicPipeline& operator=(GraphicPipeline&&) = delete;

    virtual void draw(const vk::UniqueCommandBuffer& commandBuffer) const = 0;
    virtual ~GraphicPipeline() = default;

protected:
    vk::UniquePipeline m_pipeline;
    vk::UniquePipelineLayout m_pipelineLayout;
};

class TriangleGraphicPipeline final: public GraphicPipeline {
public:
    TriangleGraphicPipeline(const vk::UniqueDevice& logicalDevice, const vk::UniqueRenderPass& renderPass)
        : GraphicPipeline() {
        m_pipelineLayout = logicalDevice->createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo());
        const auto currentDir = std::filesystem::current_path();
        const auto shaderPath = currentDir / "../../../../thyme/include/thyme/platform/shaders/spv";
        const auto shaderAbsolutePath = std::filesystem::absolute(shaderPath);
        const auto vertShader = readFile(shaderAbsolutePath / "triangle.vert.spv");
        const auto fragShader = readFile(shaderAbsolutePath / "triangle.frag.spv");
        const auto vertexShaderModule = logicalDevice->createShaderModuleUnique(
                vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlagBits(),
                                           vertShader.size(),
                                           reinterpret_cast<const uint32_t*>(vertShader.data())));
        const auto fragmentShaderModule = logicalDevice->createShaderModuleUnique(
                vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlagBits(),
                                           fragShader.size(),
                                           reinterpret_cast<const uint32_t*>(fragShader.data())));
        const auto vertexShaderStageInfo = vk::PipelineShaderStageCreateInfo(
                vk::PipelineShaderStageCreateFlagBits(), vk::ShaderStageFlagBits::eVertex, *vertexShaderModule, "main");
        const auto fragmentShaderStageInfo = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlagBits(),
                                                                               vk::ShaderStageFlagBits::eFragment,
                                                                               *fragmentShaderModule,
                                                                               "main");
        const auto shaderStages = { vertexShaderStageInfo, fragmentShaderStageInfo };


        constexpr auto dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        const auto dynamicStateCreateInfo =
                vk::PipelineDynamicStateCreateInfo(vk::PipelineDynamicStateCreateFlagBits(), dynamicStates);
        constexpr auto vertexInputStateCreateInfo =
                vk::PipelineVertexInputStateCreateInfo(vk::PipelineVertexInputStateCreateFlagBits());
        constexpr auto inputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo(
                vk::PipelineInputAssemblyStateCreateFlagBits(), vk::PrimitiveTopology::eTriangleList, vk::False);
        constexpr auto viewportState =
                vk::PipelineViewportStateCreateInfo(vk::PipelineViewportStateCreateFlagBits(), 1, nullptr, 1, nullptr);
        constexpr auto rasterizer =
                vk::PipelineRasterizationStateCreateInfo(vk::PipelineRasterizationStateCreateFlagBits(),
                                                         vk::False,
                                                         vk::False,
                                                         vk::PolygonMode::eFill,
                                                         vk::CullModeFlagBits::eBack,
                                                         vk::FrontFace::eClockwise,
                                                         vk::False,
                                                         0.0f,
                                                         0.0f,
                                                         0.0f,
                                                         1.0f);
        constexpr auto multisampling =
                vk::PipelineMultisampleStateCreateInfo(vk::PipelineMultisampleStateCreateFlagBits(),
                                                       vk::SampleCountFlagBits::e1,
                                                       vk::False,
                                                       1.0f,
                                                       nullptr,
                                                       vk::False,
                                                       vk::False);
        constexpr auto colorBlendAttachments = vk::PipelineColorBlendAttachmentState(
                vk::False,
                vk::BlendFactor::eOne,
                vk::BlendFactor::eZero,
                vk::BlendOp::eAdd,
                vk::BlendFactor::eOne,
                vk::BlendFactor::eZero,
                vk::BlendOp::eAdd,
                vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                        | vk::ColorComponentFlagBits::eB);
        const auto colorBlendStateCreateInfo =
                vk::PipelineColorBlendStateCreateInfo(vk::PipelineColorBlendStateCreateFlagBits(),
                                                      vk::False,
                                                      vk::LogicOp::eClear,
                                                      { colorBlendAttachments },
                                                      std::array{ 0.0f, 0.0f, 0.0f, 0.0f });

        m_pipeline =
                logicalDevice
                        ->createGraphicsPipelineUnique({},
                                                       vk::GraphicsPipelineCreateInfo(vk::PipelineCreateFlagBits(),
                                                                                      shaderStages,
                                                                                      &vertexInputStateCreateInfo,
                                                                                      &inputAssemblyStateCreateInfo,
                                                                                      nullptr,
                                                                                      &viewportState,
                                                                                      &rasterizer,
                                                                                      &multisampling,
                                                                                      nullptr,
                                                                                      &colorBlendStateCreateInfo,
                                                                                      &dynamicStateCreateInfo,
                                                                                      *m_pipelineLayout,
                                                                                      *renderPass,
                                                                                      0))
                        .value;
    }

    void draw(const vk::UniqueCommandBuffer& commandBuffer) const override {
        commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
        commandBuffer->draw(3, 1, 0, 0);
    }
};

}// namespace Thyme::Vulkan
