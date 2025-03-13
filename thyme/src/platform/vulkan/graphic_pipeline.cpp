#include <thyme/core/common_structs.hpp>
#include <thyme/core/texture.hpp>
#include <thyme/core/utils.hpp>
#include <thyme/platform/vulkan/graphic_pipeline.hpp>

#include <chrono>
#include <filesystem>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.hpp>


using namespace th::vulkan;
TriangleGraphicPipeline::TriangleGraphicPipeline(const Device& device, const vk::UniqueRenderPass& renderPass,
                                                 const vk::UniqueCommandPool& commandPool)
    : GraphicPipeline(), m_uniformBufferObject(device), m_uniformBufferObject2(device),
      m_texture(device, commandPool, Texture("C:\\Users\\sylwek\\Desktop\\grumpy.jpg")),
      m_texture2(device, commandPool, Texture("C:\\Users\\sylwek\\Desktop\\grumpy2.jpg")) {
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
    const auto m_descriptorSetLayouts = std::array{ *m_descriptorSetLayout, *m_descriptorSetLayout };
    m_descriptorSets = device.logicalDevice->allocateDescriptorSets(
            vk::DescriptorSetAllocateInfo(*m_descriptorPool, m_descriptorSetLayouts));

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


    m_vertexMemoryBuffers[0] =
            createBufferMemory(device, commandPool, vertices1, vk::BufferUsageFlagBits::eVertexBuffer);
    m_vertexMemoryBuffers[1] =
            createBufferMemory(device, commandPool, vertices2, vk::BufferUsageFlagBits::eVertexBuffer);
    m_indexMemoryBuffer = createBufferMemory(device, commandPool, indices, vk::BufferUsageFlagBits::eIndexBuffer);
    const auto descriptorBufferInfo = m_uniformBufferObject.getDescriptorBufferInfos();
    const auto descriptorBufferInfo2 = m_uniformBufferObject2.getDescriptorBufferInfos();
    const auto descriptorBufferInfos = std::array{ descriptorBufferInfo, descriptorBufferInfo2 };
    const auto descriptorImageInfos = std::array{ vk::DescriptorImageInfo(*m_texture.sampler,
                                                                          *m_texture.imageMemory.imageView,
                                                                          vk::ImageLayout::eShaderReadOnlyOptimal),
                                                  vk::DescriptorImageInfo(*m_texture2.sampler,
                                                                          *m_texture2.imageMemory.imageView,
                                                                          vk::ImageLayout::eShaderReadOnlyOptimal) };
    for (const auto [descriptorSet, descBufferInfo, descriptorImageInfo] :
         std::views::zip(m_descriptorSets, descriptorBufferInfos, descriptorImageInfos)) {
        const auto writeDescriptorSets = std::array{
            vk::WriteDescriptorSet(descriptorSet, 0, 0, vk::DescriptorType::eUniformBuffer, {}, { descBufferInfo }),
            vk::WriteDescriptorSet(
                    descriptorSet, 1, 0, vk::DescriptorType::eCombinedImageSampler, { descriptorImageInfo }, {})
        };
        device.logicalDevice->updateDescriptorSets(writeDescriptorSets, {});
    }
}

void TriangleGraphicPipeline::updateUBO(const vk::Extent2D& extend) const {
    using namespace std::chrono;
    static const auto startTime = high_resolution_clock::now();
    const auto currentTime = high_resolution_clock::now();
    const auto deltaTime = duration<float, seconds::period>(currentTime - startTime).count();
    const auto view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    auto proj =
            glm::perspective(glm::radians(45.0f), static_cast<float>(extend.width) / static_cast<float>(extend.height), 0.1f, 10.0f);
    proj[1][1] *= -1.0f;
    const auto ubo =
            MVP{ .model = glm::rotate(glm::mat4(1.0f), deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                 .view = view,
                 .proj = proj };
    m_uniformBufferObject.update(ubo);

    const auto ubo2 =
            MVP{ .model = glm::rotate(glm::mat4(1.0f), -deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                 .view = view,
                 .proj = proj };
    m_uniformBufferObject2.update(ubo2);
}