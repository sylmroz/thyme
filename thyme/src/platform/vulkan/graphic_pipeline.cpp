#include <thyme/core/common_structs.hpp>
#include <thyme/core/utils.hpp>
#include <thyme/platform/vulkan/graphic_pipeline.hpp>
#include <thyme/scene/model.hpp>

#include <chrono>
#include <filesystem>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.hpp>


namespace th::vulkan {

ScenePipeline::ScenePipeline(const Device& device, const vk::PipelineRenderingCreateInfo& pipelineRenderingCreateInfo,
                             const vk::CommandPool commandPool, scene::ModelStorage& modelStorage,
                             scene::Camera& camera)
    : m_camera{ camera } {

    for (const auto& model : modelStorage) {
        m_models.emplace_back(model, device, commandPool);
    }
    constexpr auto uboBinding =
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
    constexpr auto samplerLayoutBinding = vk::DescriptorSetLayoutBinding(
            1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    constexpr auto bindings = std::array{ uboBinding, samplerLayoutBinding };

    m_descriptorSetLayout = device.logicalDevice->createDescriptorSetLayoutUnique(
            vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags{}, bindings));
    m_pipelineLayout = device.logicalDevice->createPipelineLayoutUnique(
            vk::PipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), 1, &(m_descriptorSetLayout.get())));
    m_descriptorPool = createDescriptorPool(
            device.logicalDevice.get(),
            { vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, m_models.size()),
              vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, m_models.size()) });

    const auto descriptorSetLayouts = std::vector{ m_models.size(), m_descriptorSetLayout.get() };
    m_descriptorSets = device.logicalDevice->allocateDescriptorSets(
            vk::DescriptorSetAllocateInfo(m_descriptorPool.get(), descriptorSetLayouts));

    const auto currentDir = std::filesystem::current_path();
    const auto shaderPath = currentDir / "../../../../thyme/include/thyme/platform/shaders/spv";
    const auto shaderAbsolutePath = std::filesystem::absolute(shaderPath);
    const auto vertShader = readFile(shaderAbsolutePath / "triangle.vert.spv");
    const auto fragShader = readFile(shaderAbsolutePath / "triangle.frag.spv");
    const auto vertexShaderModule = device.logicalDevice->createShaderModuleUnique(vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlagBits(), vertShader.size(), reinterpret_cast<const uint32_t*>(vertShader.data())));
    const auto fragmentShaderModule = device.logicalDevice->createShaderModuleUnique(vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlagBits(), fragShader.size(), reinterpret_cast<const uint32_t*>(fragShader.data())));
    const auto vertexShaderStageInfo = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlagBits(),
                                                                         vk::ShaderStageFlagBits::eVertex,
                                                                         vertexShaderModule.get(),
                                                                         "main");
    const auto fragmentShaderStageInfo = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlagBits(),
                                                                           vk::ShaderStageFlagBits::eFragment,
                                                                           fragmentShaderModule.get(),
                                                                           "main");
    const auto shaderStages = std::vector{ vertexShaderStageInfo, fragmentShaderStageInfo };

    m_pipeline = createGraphicsPipeline(
            GraphicPipelineCreateInfo{ .logicalDevice = device.logicalDevice.get(),
                                       .pipelineLayout = m_pipelineLayout.get(),
                                       .samples = device.maxMsaaSamples,
                                       .pipelineRenderingCreateInfo = pipelineRenderingCreateInfo,
                                       .shaderStages = shaderStages });

    for (const auto [descriptorSet, model] : std::views::zip(m_descriptorSets, m_models)) {
        const auto descriptorBufferInfo = model.getUniformBufferObject().getDescriptorBufferInfos();
        const auto descriptorImageInfo =
                model.getTexture().getDescriptorImageInfo(vk::ImageLayout::eShaderReadOnlyOptimal);
        const auto writeDescriptorSets = std::array{
            vk::WriteDescriptorSet(
                    descriptorSet, 0, 0, vk::DescriptorType::eUniformBuffer, {}, { descriptorBufferInfo }),
            vk::WriteDescriptorSet(
                    descriptorSet, 1, 0, vk::DescriptorType::eCombinedImageSampler, { descriptorImageInfo }, {})
        };
        device.logicalDevice->updateDescriptorSets(writeDescriptorSets, {});
    }
}

void ScenePipeline::draw(const vk::CommandBuffer commandBuffer) const {
    updateUBO();

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.get());
    for (const auto& [model, descriptor] : std::views::zip(m_models, m_descriptorSets)) {
        commandBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, { descriptor }, {});
        model.draw(commandBuffer);
    }
}

void ScenePipeline::updateUBO() const {
    using namespace std::chrono;
    static const auto startTime = high_resolution_clock::now();
    const auto currentTime = high_resolution_clock::now();
    const auto deltaTime = duration<float, seconds::period>(currentTime - startTime).count();
    const auto& view = m_camera.getViewMatrix();
    const auto& proj = m_camera.getProjectionMatrix();
    const auto ubo = renderer::MVP{
        .model = glm::rotate(glm::mat4(1.0f), deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .view = view,
        .proj = proj
    };
    m_models[0].getUniformBufferObject().update(ubo);

    const auto ubo2 = renderer::MVP{
        .model = glm::rotate(glm::mat4(1.0f), -deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .view = view,
        .proj = proj
    };
    m_models[1].getUniformBufferObject().update(ubo2);
}

}// namespace th::vulkan