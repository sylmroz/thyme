#include <filesystem>

#include <vulkan/vulkan.hpp>

import thyme.platform.vulkan;
import thyme.core.utils;

using namespace Thyme::Vulkan;
TriangleGraphicPipeline::TriangleGraphicPipeline(const vk::UniqueDevice& logicalDevice,
                                                 const vk::UniqueRenderPass& renderPass)
    : GraphicPipeline() {
    m_pipelineLayout = logicalDevice->createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo());
    const auto currentDir = std::filesystem::current_path();
    const auto shaderPath = currentDir / "../../../../thyme/include/thyme/platform/shaders/spv";
    const auto shaderAbsolutePath = std::filesystem::absolute(shaderPath);
    const auto vertShader = readFile(shaderAbsolutePath / "triangle.vert.spv");
    const auto fragShader = readFile(shaderAbsolutePath / "triangle.frag.spv");
    const auto vertexShaderModule = logicalDevice->createShaderModuleUnique(vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlagBits(), vertShader.size(), reinterpret_cast<const uint32_t*>(vertShader.data())));
    const auto fragmentShaderModule = logicalDevice->createShaderModuleUnique(vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlagBits(), fragShader.size(), reinterpret_cast<const uint32_t*>(fragShader.data())));
    const auto vertexShaderStageInfo = vk::PipelineShaderStageCreateInfo(
            vk::PipelineShaderStageCreateFlagBits(), vk::ShaderStageFlagBits::eVertex, *vertexShaderModule, "main");
    const auto fragmentShaderStageInfo = vk::PipelineShaderStageCreateInfo(
            vk::PipelineShaderStageCreateFlagBits(), vk::ShaderStageFlagBits::eFragment, *fragmentShaderModule, "main");
    const auto shaderStages = std::vector{ vertexShaderStageInfo, fragmentShaderStageInfo };

    m_pipeline = createGraphicsPipeline(GraphicPipelineCreateInfo{ .logicalDevice = logicalDevice,
                                                                   .renderPass = renderPass,
                                                                   .pipelineLayout = m_pipelineLayout,
                                                                   .samples = vk::SampleCountFlagBits::e1,
                                                                   .shaderStages = shaderStages });
}