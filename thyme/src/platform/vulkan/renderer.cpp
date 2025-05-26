#include <thyme/core/logger.hpp>
#include <thyme/platform/vulkan/renderer.hpp>

#include <vulkan/vulkan.hpp>

namespace th::vulkan {

VulkanRenderTarget::VulkanRenderTarget(const vk::ImageView imageView, const vk::ImageView resolvedImageView,
                                       const vk::ImageLayout imageLayout, const vk::AttachmentLoadOp loadOp,
                                       const vk::AttachmentStoreOp storeOp, const vk::ClearValue clearValue) {
    m_attachmentInfo = vk::RenderingAttachmentInfo(imageView,
                                                   imageLayout,
                                                   vk::ResolveModeFlagBits::eAverage,
                                                   resolvedImageView,
                                                   imageLayout,
                                                   loadOp,
                                                   storeOp,
                                                   clearValue);
}

VulkanRenderTarget::VulkanRenderTarget(const vk::ImageView imageView, const vk::ImageLayout imageLayout,
                                       const vk::AttachmentLoadOp loadOp, const vk::AttachmentStoreOp storeOp,
                                       const vk::ClearValue clearValue) {
    m_attachmentInfo = vk::RenderingAttachmentInfo(
            imageView, imageLayout, vk::ResolveModeFlagBits::eNone, {}, {}, loadOp, storeOp, clearValue);
}

// VulkanRenderingInfo::VulkanRenderingInfo(VulkanSwapChain* swapChain, const vk::SampleCountFlagBits samples)
//     : m_swapChain{ swapChain }, m_samples{ samples } {
//     m_colorMemory = ColorImageMemory()
// }

VulkanRenderer::VulkanRenderer(const VulkanDevice& device, VulkanSwapChain& swapChain,
                               scene::ModelStorage& modelStorage, scene::Camera& camera, Gui& gui,
                               const VulkanGraphicContext& context,
                               VulkanCommandBuffersPool* commandBuffersPool) noexcept
    : m_gui{ gui }, m_swapChain{ swapChain }, m_commandBuffersPool{ commandBuffersPool },
      m_depthImageMemory{ device, swapChain.getSwapChainExtent(), context.depthFormat, device.maxMsaaSamples },
      m_colorImageMemory{ device,
                          swapChain.getSwapChainExtent(),
                          context.surfaceFormat.format,
                          device.maxMsaaSamples } {
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
    m_colorImageMemory.resize(m_swapChain.getSwapChainExtent());
    m_depthImageMemory.resize(m_swapChain.getSwapChainExtent());
    m_swapChain.prepareRenderMode();
    const auto swapChainImage = m_swapChain.getCurrentSwapChainFrame();
    constexpr auto clearColorValues = vk::ClearValue(vk::ClearColorValue(1.0f, 0.0f, 1.0f, 1.0f));
    constexpr auto depthClearValue = vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0));

    // const auto colorRenderTarget = VulkanRenderTarget(m_colorImageMemory.getImageView(),
    //                                                   swapChainImage.imageView,
    //                                                   vk::ImageLayout::eColorAttachmentOptimal,
    //                                                   vk::AttachmentLoadOp::eClear,
    //                                                   vk::AttachmentStoreOp::eStore,
    //                                                   clearColorValues);

    const auto colorAttachment = vk::RenderingAttachmentInfo(m_colorImageMemory.getImageView(),
                                                             vk::ImageLayout::eColorAttachmentOptimal,
                                                             vk::ResolveModeFlagBits::eAverage,
                                                             swapChainImage.imageView,
                                                             vk::ImageLayout::eColorAttachmentOptimal,
                                                             vk::AttachmentLoadOp::eClear,
                                                             vk::AttachmentStoreOp::eStore,
                                                             clearColorValues);

    const auto depthAttachment = vk::RenderingAttachmentInfo(m_depthImageMemory.getImageView(),
                                                             vk::ImageLayout::eDepthAttachmentOptimal,
                                                             vk::ResolveModeFlagBits::eNone,
                                                             {},
                                                             {},
                                                             vk::AttachmentLoadOp::eClear,
                                                             vk::AttachmentStoreOp::eDontCare,
                                                             depthClearValue);

    const auto renderingInfo = vk::RenderingInfo(vk::RenderingFlagBits{},
                                                 vk::Rect2D(vk::Offset2D(0, 0), m_swapChain.getSwapChainExtent()),
                                                 1,
                                                 0,
                                                 { colorAttachment },
                                                 &depthAttachment);
    const auto commandBuffer = m_commandBuffersPool->get().getBuffer();
    commandBuffer.beginRendering(renderingInfo);
    for (const auto& pipeline : m_pipelines) {
        pipeline->draw(commandBuffer);
    }
    m_gui.start();
    m_gui.draw(commandBuffer);
    commandBuffer.endRendering();
    m_swapChain.preparePresentMode();
    m_swapChain.submitFrame();
}

}// namespace th::vulkan