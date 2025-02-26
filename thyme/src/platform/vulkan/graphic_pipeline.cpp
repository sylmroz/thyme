#include <thyme/core/common_structs.hpp>
#include <thyme/core/utils.hpp>
#include <thyme/platform/vulkan/graphic_pipeline.hpp>
#include <thyme/renderer/models.hpp>
#include <thyme/core/texture.hpp>
#include <thyme/platform/vulkan/uniform_buffer_object.hpp>

#include <chrono>
#include <filesystem>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.hpp>


using namespace Thyme::Vulkan;
TriangleGraphicPipeline::TriangleGraphicPipeline(const Device& device, const vk::UniqueRenderPass& renderPass,
                                                 const vk::UniqueCommandPool& commandPool)
    : m_uniformBufferObject(device), m_texture(device, commandPool, Texture("C:\\Users\\sylwek\\Desktop\\grumpy.jpg")), GraphicPipeline() {
    constexpr auto uboBinding =
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
    constexpr auto samplerLayoutBinding = vk::DescriptorSetLayoutBinding(
            1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    constexpr auto bindings = std::array{ uboBinding, samplerLayoutBinding };
    m_descriptorSetLayout = device.logicalDevice->createDescriptorSetLayoutUnique(
            vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags{}, bindings));
    m_pipelineLayout = device.logicalDevice->createPipelineLayoutUnique(
            vk::PipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), 1, &(*m_descriptorSetLayout)));
    m_descriptorPool = createDescriptorPool(device.logicalDevice,
                                            { vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 2),
                                              vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 2) });
    const std::array descriptorSetLayouts = { *m_descriptorSetLayout, *m_descriptorSetLayout };
    m_descriptorSets = device.logicalDevice->allocateDescriptorSets(
            vk::DescriptorSetAllocateInfo(*m_descriptorPool, descriptorSetLayouts));

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
                                                                   .samples = device.maxMsaaSamples,
                                                                   .shaderStages = shaderStages });


    m_vertexMemoryBuffers[0] = createBufferMemory(device, commandPool, vertices1, vk::BufferUsageFlagBits::eVertexBuffer);
    m_vertexMemoryBuffers[1] = createBufferMemory(device, commandPool, vertices2, vk::BufferUsageFlagBits::eVertexBuffer);
    m_indexMemoryBuffer = createBufferMemory(device, commandPool, indices, vk::BufferUsageFlagBits::eIndexBuffer);


    const auto& [imageMemory, sampler] = m_texture;
    const auto descriptorBufferInfos = m_uniformBufferObject.getDescriptorBufferInfos();

    //
    for (auto ub : std::views::zip(descriptorBufferInfos, m_descriptorSets)) {
        const auto descriptorImageInfo =
                vk::DescriptorImageInfo(*sampler, *imageMemory.imageView, vk::ImageLayout::eShaderReadOnlyOptimal);
        const auto descriptorSet = std::get<1>(ub);
        const auto writeDescriptorSets = std::array{
            vk::WriteDescriptorSet(descriptorSet, 0, 0, vk::DescriptorType::eUniformBuffer, {}, { std::get<0>(ub) }),
            vk::WriteDescriptorSet(
                    descriptorSet, 1, 0, vk::DescriptorType::eCombinedImageSampler, { descriptorImageInfo }, {})
        };
        device.logicalDevice->updateDescriptorSets(writeDescriptorSets, {});
    }
}

void TriangleGraphicPipeline::updateUBO(const uint32_t currentImage, const vk::Extent2D& extend) const {
    using namespace std::chrono;
    static const auto startTime = high_resolution_clock::now();
    const auto currentTime = high_resolution_clock::now();
    const auto deltaTime = duration<float, seconds::period>(currentTime - startTime).count();
    auto ubo = MVP{
        .model = glm::rotate(glm::mat4(1.0f), deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .view = glm::lookAt(glm::vec3(2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .proj = glm::perspective(glm::radians(45.0f), extend.width / static_cast<float>(extend.height), 0.1f, 10.0f)
    };
    ubo.proj[1][1] *= -1.0f;
    m_uniformBufferObject.update(currentImage, ubo);
}