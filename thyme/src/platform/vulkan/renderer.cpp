#include <thyme/core/logger.hpp>
#include <thyme/platform/vulkan/renderer.hpp>

#include <vulkan/vulkan.hpp>

namespace th::vulkan {

VulkanRenderer::VulkanRenderer(const VulkanDevice& device, VulkanSwapChain& swapChain,
                               scene::ModelStorage& modelStorage, scene::Camera& camera, Gui& gui,
                               const VulkanGraphicContext& context,
                               VulkanCommandBuffersPool* commandBuffersPool) noexcept
    : m_device{ device }, m_gui{ gui }, m_swapChain{ swapChain }, m_commandBuffersPool{ commandBuffersPool } {
    m_pipelines.emplace_back(std::make_unique<ScenePipeline>(
            device,
            vk::PipelineRenderingCreateInfo(0, { context.surfaceFormat.format }, context.depthFormat),
            modelStorage,
            camera));
}

void VulkanRenderer::draw() {
    if (!m_swapChain.prepareFrame()) {
        return;
    }

    m_swapChain.prepareRenderMode();

    auto& commandBuffer = m_commandBuffersPool->get();
    for (const auto& pipeline : m_pipelines) {
        pipeline->draw(commandBuffer.getBuffer());
    }
    m_gui.start();
    m_gui.draw(commandBuffer.getBuffer());
    m_swapChain.preparePresentMode();
    m_swapChain.submitFrame();
}

}// namespace th::vulkan