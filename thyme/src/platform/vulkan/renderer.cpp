#include <thyme/core/logger.hpp>
#include <thyme/platform/vulkan/renderer.hpp>

#include <vulkan/vulkan.hpp>

namespace th::vulkan {

VulkanRenderer::VulkanRenderer(const VulkanDevice& device, VulkanSwapChain& swapChain,
                               scene::ModelStorage& modelStorage, scene::Camera& camera, Gui& gui,
                               const VulkanGraphicContext& context) noexcept
    : m_device{ device }, m_gui{ gui }, m_swapChain{ swapChain },
      m_commandBuffers{ m_device.logicalDevice.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
              device.commandPool, vk::CommandBufferLevel::ePrimary, context.imageCount)) } {
    m_pipelines.emplace_back(std::make_unique<ScenePipeline>(
            device,
            vk::PipelineRenderingCreateInfo(
                    0, { context.surfaceFormat.format }, findDepthFormat(device.physicalDevice)),
            modelStorage,
            camera));
}

void VulkanRenderer::draw() {
    if (!m_swapChain.prepareFrame()) {
        return;
    }

    const auto commandBuffer = m_commandBuffers[m_commandBufferIndex].get();
    ++m_commandBufferIndex %= m_commandBuffers.size();
    commandBuffer.reset();
    commandBuffer.begin(vk::CommandBufferBeginInfo());
    m_swapChain.prepareRenderMode(commandBuffer);
    for (const auto& pipeline : m_pipelines) {
        pipeline->draw(commandBuffer);
    }
    m_gui.start();
    m_gui.draw(commandBuffer);
    m_swapChain.preparePresentMode(commandBuffer);

    commandBuffer.end();

    m_swapChain.renderGraphic(commandBuffer);

    m_swapChain.submitFrame();
}

}// namespace th::vulkan