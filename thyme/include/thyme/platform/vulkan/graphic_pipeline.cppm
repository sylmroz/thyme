module;

#include <filesystem>

#include <vulkan/vulkan.hpp>

import thyme.core.utils;

export module thyme.platform.vulkan:graphic_pipeline;
import :utils;

export namespace Thyme::Vulkan {

class GraphicPipeline {
public:
    GraphicPipeline() = default;

    GraphicPipeline(const GraphicPipeline&) = delete;
    GraphicPipeline(GraphicPipeline&&) = delete;

    GraphicPipeline& operator=(const GraphicPipeline&) = delete;
    GraphicPipeline& operator=(GraphicPipeline&&) = delete;

    virtual void draw(const vk::UniqueCommandBuffer& commandBuffer) const = 0;
    virtual ~GraphicPipeline() = default;
};

class TriangleGraphicPipeline final: public GraphicPipeline {
public:
    explicit TriangleGraphicPipeline(const vk::UniqueDevice& logicalDevice, const vk::UniqueRenderPass& renderPass)
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
        const auto shaderStages = std::vector{ vertexShaderStageInfo, fragmentShaderStageInfo };

        m_pipeline = createGraphicsPipeline(GraphicPipelineCreateInfo{ .logicalDevice = logicalDevice,
                                                                       .renderPass = renderPass,
                                                                       .pipelineLayout = m_pipelineLayout,
                                                                       .samples = vk::SampleCountFlagBits::e1,
                                                                       .shaderStages = shaderStages });
    }

    inline virtual void draw(const vk::UniqueCommandBuffer& commandBuffer) const override {
        commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
        commandBuffer->draw(3, 1, 0, 0);
    }

private:
    vk::UniquePipeline m_pipeline;
    vk::UniquePipelineLayout m_pipelineLayout;
};
}// namespace Thyme::Vulkan
