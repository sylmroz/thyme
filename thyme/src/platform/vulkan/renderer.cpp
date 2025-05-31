#include <thyme/core/logger.hpp>
#include <thyme/platform/vulkan/renderer.hpp>

#include <vulkan/vulkan.hpp>

namespace th::vulkan {

VulkanRenderTarget::VulkanRenderTarget(const vk::ImageView imageView, const vk::ImageView resolvedImageView,
                                       const vk::ImageLayout imageLayout, const vk::AttachmentLoadOp loadOp,
                                       const vk::AttachmentStoreOp storeOp, const vk::ClearValue clearValue) {
    m_attachmentInfo = vk::RenderingAttachmentInfo{
        .imageView = imageView,
        .imageLayout = imageLayout,
        .resolveMode = vk::ResolveModeFlagBits::eAverage,
        .resolveImageView = resolvedImageView,
        .resolveImageLayout = imageLayout,
        .loadOp = loadOp,
        .storeOp = storeOp,
        .clearValue = clearValue,
    };
}

VulkanRenderTarget::VulkanRenderTarget(const vk::ImageView imageView, const vk::ImageLayout imageLayout,
                                       const vk::AttachmentLoadOp loadOp, const vk::AttachmentStoreOp storeOp,
                                       const vk::ClearValue clearValue) {
    m_attachmentInfo = vk::RenderingAttachmentInfo{
        .imageView = imageView,
        .imageLayout = imageLayout,
        .resolveMode = vk::ResolveModeFlagBits::eNone,
        .loadOp = loadOp,
        .storeOp = storeOp,
        .clearValue = clearValue,
    };
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
            vk::PipelineRenderingCreateInfo{ .viewMask = 0,
                                             .colorAttachmentCount = 1,
                                             .pColorAttachmentFormats = &context.surfaceFormat.format,
                                             .depthAttachmentFormat = context.depthFormat },
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


    const auto colorAttachment = vk::RenderingAttachmentInfo{
        .imageView = m_colorImageMemory.getImageView(),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .resolveMode = vk::ResolveModeFlagBits::eAverage,
        .resolveImageView = swapChainImage.imageView,
        .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColorValues,
    };

    const auto depthAttachment = vk::RenderingAttachmentInfo{
        .imageView = m_depthImageMemory.getImageView(),
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .resolveMode = vk::ResolveModeFlagBits::eNone,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = depthClearValue,
    };

    const auto renderingInfo = vk::RenderingInfo{
        .renderArea =
                vk::Rect2D{ .offset = vk::Offset2D{ .x = 0, .y = 0 }, .extent = m_swapChain.getSwapChainExtent() },
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
        .pDepthAttachment = &depthAttachment,
    };
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