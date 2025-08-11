#include <thyme/core/common_structs.hpp>
#include <thyme/core/utils.hpp>
#include <thyme/platform/vulkan/graphic_pipeline.hpp>
#include <thyme/platform/vulkan/uniform_buffer_object.hpp>
#include <thyme/platform/vulkan/vulkan_texture.hpp>
#include <thyme/renderer/structs.hpp>
#include <thyme/scene/model.hpp>

#include <filesystem>

namespace th::vulkan {

ScenePipeline::ScenePipeline(const VulkanDevice& device,
                             const vk::PipelineRenderingCreateInfo& pipelineRenderingCreateInfo,
                             std::vector<VulkanModel>& models,
                             const UniformBufferObject<renderer::CameraMatrices>& cameraMatrices) {
    constexpr auto uboBinding =
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
    constexpr auto camarauboBinding =
            vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
    constexpr auto samplerLayoutBinding = vk::DescriptorSetLayoutBinding(
            2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    constexpr auto bindings = std::array{ uboBinding, camarauboBinding, samplerLayoutBinding };

    m_descriptorSetLayout = device.logicalDevice.createDescriptorSetLayoutUnique(
            vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateInfo{
                    .bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data() }));
    m_descriptorPool =
            createDescriptorPool(device.logicalDevice,
                                 { vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, models.size()),
                                   vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, models.size()),
                                   vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, models.size()) });

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
                                    .dstBinding = 0,
                                    .dstArrayElement = 0,
                                    .descriptorCount = 1,
                                    .descriptorType = vk::DescriptorType::eUniformBuffer,
                                    .pBufferInfo = &cameraDescriptorBufferInfo,
                            },
                            vk::WriteDescriptorSet{
                                    .dstSet = descriptorSet,
                                    .dstBinding = 1,
                                    .dstArrayElement = 0,
                                    .descriptorCount = 1,
                                    .descriptorType = vk::DescriptorType::eUniformBuffer,
                                    .pBufferInfo = &descriptorBufferInfo,
                            },
                            vk::WriteDescriptorSet{
                                    .dstSet = descriptorSet,
                                    .dstBinding = 2,
                                    .dstArrayElement = 0,
                                    .descriptorCount = 1,
                                    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                    .pImageInfo = &descriptorImageInfo,
                            } };
        device.logicalDevice.updateDescriptorSets(writeDescriptorSets, {});
    }

    const auto currentDir = std::filesystem::current_path();
    const auto shaderPath = currentDir / "../../../../thyme/include/thyme/platform/shaders/spv";
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
    m_pipeline = createGraphicsPipeline(
            GraphicPipelineCreateInfo{ .logicalDevice = device.logicalDevice,
                                       .pipelineLayout = m_pipelineLayout.get(),
                                       .samples = device.maxMsaaSamples,
                                       .pipelineRenderingCreateInfo = pipelineRenderingCreateInfo,
                                       .shaderStages = shaderStages });
}

void ScenePipeline::draw(const vk::CommandBuffer commandBuffer, const std::vector<VulkanModel>& models) const {
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.get());
    for (const auto& [model, descriptor] : std::views::zip(models, m_descriptorSets)) {
        commandBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, { descriptor }, {});
        model.draw(commandBuffer);
    }
}

}// namespace th::vulkan