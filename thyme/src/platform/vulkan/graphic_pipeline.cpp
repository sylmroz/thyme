#include <filesystem>

#include <vulkan/vulkan.hpp>

import thyme.platform.vulkan;
import thyme.core.utils;

using namespace Thyme::Vulkan;
TriangleGraphicPipeline::TriangleGraphicPipeline(const Device& device, const vk::UniqueRenderPass& renderPass)
    : GraphicPipeline() {
    m_pipelineLayout = device.logicalDevice->createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo());
    const auto currentDir = std::filesystem::current_path();
    const auto shaderPath = currentDir / "../../../../thyme/include/thyme/platform/shaders/spv";
    const auto shaderAbsolutePath = std::filesystem::absolute(shaderPath);
    const auto vertShader = readFile(shaderAbsolutePath / "triangle.vert.spv");
    const auto fragShader = readFile(shaderAbsolutePath / "triangle.frag.spv");
    const auto vertexShaderModule = device.logicalDevice->createShaderModuleUnique(vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlagBits(), vertShader.size(), reinterpret_cast<const uint32_t*>(vertShader.data())));
    const auto fragmentShaderModule = device.logicalDevice->createShaderModuleUnique(vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlagBits(), fragShader.size(), reinterpret_cast<const uint32_t*>(fragShader.data())));
    const auto vertexShaderStageInfo = vk::PipelineShaderStageCreateInfo(
            vk::PipelineShaderStageCreateFlagBits(), vk::ShaderStageFlagBits::eVertex, *vertexShaderModule, "main");
    const auto fragmentShaderStageInfo = vk::PipelineShaderStageCreateInfo(
            vk::PipelineShaderStageCreateFlagBits(), vk::ShaderStageFlagBits::eFragment, *fragmentShaderModule, "main");
    const auto shaderStages = std::vector{ vertexShaderStageInfo, fragmentShaderStageInfo };

    m_pipeline = createGraphicsPipeline(GraphicPipelineCreateInfo{ .logicalDevice = device.logicalDevice,
                                                                   .renderPass = renderPass,
                                                                   .pipelineLayout = m_pipelineLayout,
                                                                   .samples = vk::SampleCountFlagBits::e1,
                                                                   .shaderStages = shaderStages });

    m_vertexBuffer =
            device.logicalDevice->createBufferUnique(vk::BufferCreateInfo(vk::BufferCreateFlagBits(),
                                                                          vertices.size() * sizeof(vertices[0]),
                                                                          vk::BufferUsageFlagBits::eVertexBuffer,
                                                                          vk::SharingMode::eExclusive));

    vk::MemoryRequirements memoryRequirements;
    device.logicalDevice->getBufferMemoryRequirements(*m_vertexBuffer, &memoryRequirements);

    m_vertexBufferMemory = device.logicalDevice->allocateMemoryUnique(vk::MemoryAllocateInfo(
            memoryRequirements.size,
            findMemoryType(device.physicalDevice,
                           memoryRequirements.memoryTypeBits,
                           vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)));

    device.logicalDevice->bindBufferMemory(*m_vertexBuffer, *m_vertexBufferMemory, 0);
    void* mappedMemory = nullptr;
    [[maybe_unused]] const auto result = device.logicalDevice->mapMemory(
            *m_vertexBufferMemory, 0, vertices.size() * sizeof(vertices[0]), vk::MemoryMapFlags(), &mappedMemory);
    memcpy(mappedMemory, vertices.data(), vertices.size() * sizeof(vertices[0]));
    device.logicalDevice->unmapMemory(*m_vertexBufferMemory);

}