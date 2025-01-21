#include <chrono>
#include <filesystem>

#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>

import thyme.platform.vulkan;
import thyme.core.utils;

import thyme.renderer.models;


using namespace Thyme::Vulkan;
TriangleGraphicPipeline::TriangleGraphicPipeline(const Device& device, const vk::UniqueRenderPass& renderPass,
                                                 const vk::UniqueCommandPool& commandPool)
    : GraphicPipeline() {
    constexpr auto binding =
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
    m_descriptorSetLayout = device.logicalDevice->createDescriptorSetLayoutUnique(
            vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags{}, { binding }));
    m_pipelineLayout = device.logicalDevice->createPipelineLayoutUnique(
            vk::PipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), 1, &(*m_descriptorSetLayout)));
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

    m_vertexMemoryBuffer = createMemoryBuffer(device, commandPool, vertices, vk::BufferUsageFlagBits::eVertexBuffer);
    m_indexMemoryBuffer = createMemoryBuffer(device, commandPool, indices, vk::BufferUsageFlagBits::eIndexBuffer);

    for (auto memoryBufferMap : std::views::zip(m_uniformMemoryBuffer, m_mappedMemoryBuffer)) {
        constexpr auto ubSize = sizeof(Renderer::UniformBufferObject);
        auto& memoryBuffer = std::get<0>(memoryBufferMap);
        memoryBuffer = createMemoryBuffer(device,
                                          ubSize,
                                          vk::BufferUsageFlagBits::eUniformBuffer,
                                          vk::MemoryPropertyFlagBits::eHostVisible
                                                  | vk::MemoryPropertyFlagBits::eHostCoherent);
        [[maybe_unused]] const auto result = device.logicalDevice->mapMemory(
                *memoryBuffer.memory, 0, ubSize, vk::MemoryMapFlags(), &std::get<1>(memoryBufferMap));
    }

    m_descriptorPool = createDescriptorPool(device.logicalDevice,
                                            { vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 2) });

    const std::array descriptorSetLayouts = { *m_descriptorSetLayout, *m_descriptorSetLayout };

    m_descriptorSets = device.logicalDevice->allocateDescriptorSets(
            vk::DescriptorSetAllocateInfo(*m_descriptorPool, descriptorSetLayouts));

    std::array<vk::WriteDescriptorSet, 2> writeDescriptorSets;
    for (auto ub : std::views::zip(m_uniformMemoryBuffer, m_descriptorSets, writeDescriptorSets)) {
        const auto descriptorBufferInfo =
                vk::DescriptorBufferInfo(*std::get<0>(ub).buffer, 0, sizeof(Renderer::UniformBufferObject));
        std::get<2>(ub) = vk::WriteDescriptorSet(
                std::get<1>(ub), 0, 0, vk::DescriptorType::eUniformBuffer, {}, { descriptorBufferInfo });
    }
    device.logicalDevice->updateDescriptorSets(writeDescriptorSets, {});
}

void TriangleGraphicPipeline::updateUBO(const uint32_t currentImage, const vk::Extent2D& extend) const {
    using namespace std::chrono;
    static const auto startTime = high_resolution_clock::now();
    const auto currentTime = high_resolution_clock::now();
    const auto deltaTime = duration<float, seconds::period>(currentTime - startTime).count();
    Renderer::UniformBufferObject ubo{
        .model = glm::rotate(glm::mat4(1.0f), deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .view = glm::lookAt(glm::vec3(2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .proj = glm::perspective(glm::radians(45.0f), extend.width / static_cast<float>(extend.height), 0.1f, 10.0f)
    };
    ubo.proj[1][1] *= -1.0f;
    memcpy(m_mappedMemoryBuffer[currentImage], &ubo, sizeof(ubo));
}