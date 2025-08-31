module;

#include <memory>
#include <ranges>

module th.render_system.vulkan;


namespace th {

RenderTarget::RenderTarget(const vk::ImageView imageView, const vk::ImageView resolvedImageView,
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

RenderTarget::RenderTarget(const vk::ImageView imageView, const vk::ImageLayout imageLayout,
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

void updateUBO(const Camera& camera, const VulkanUniformBuffer<CameraMatrices>& cameraMatrices,
               std::vector<VulkanModel>& models, ModelStorage& model_storage) {
    cameraMatrices.update(CameraMatrices{ .view = camera.getViewMatrix(), .projection = camera.getProjectionMatrix() });
    for (auto [vlkModel, model] : std::views::zip(models, model_storage)) {
        model.animate();
        vlkModel.getUniformBufferObject().update(model.transformation.getTransformMatrix());
    }
}

VulkanRenderer::VulkanRenderer(const VulkanDevice& device, VulkanSwapChain& swapChain, ModelStorage& modelStorage,
                               Camera& camera, Gui& gui, const VulkanGraphicContext& context,
                               VulkanCommandBuffersPool& commandBuffersPool) noexcept
    : m_gui{ gui }, m_swapChain{ swapChain }, m_commandBuffersPool{ commandBuffersPool },
      m_depthImageMemory{ device, swapChain.getSwapChainExtent(), context.depthFormat, device.maxMsaaSamples },
      m_colorImageMemory{ device, swapChain.getSwapChainExtent(), context.colorFormat, device.maxMsaaSamples },
      m_resolveColorImageMemory{ device, swapChain.getSwapChainExtent(), context.colorFormat }, m_camera{ camera },
      m_modelStorage{ modelStorage }, m_cameraMatrices{ device } {

    for (const auto& model : modelStorage) {
        m_models.emplace_back(model, device);
    }
    m_pipelines.emplace_back(std::make_unique<VulkanScenePipeline>(
            device,
            vk::PipelineRenderingCreateInfo{ .viewMask = 0,
                                             .colorAttachmentCount = 1,
                                             .pColorAttachmentFormats = &context.colorFormat,
                                             .depthAttachmentFormat = context.depthFormat },
            m_models,
            m_cameraMatrices));
}

void VulkanRenderer::draw() {
    if (!m_swapChain.prepareFrame()) {
        return;
    }
    updateUBO(m_camera, m_cameraMatrices, m_models, m_modelStorage);
    m_colorImageMemory.resize(m_swapChain.getSwapChainExtent());
    m_resolveColorImageMemory.resize(m_swapChain.getSwapChainExtent());
    m_gui.addTexture(m_resolveColorImageMemory.getImageView(), m_resolveColorImageMemory.getSampler());
    m_depthImageMemory.resize(m_swapChain.getSwapChainExtent());

    const auto commandBuffer = m_commandBuffersPool.get().getBuffer();
    setCommandBufferFrameSize(commandBuffer, m_swapChain.getSwapChainExtent());
    constexpr auto clearColorValues = vk::ClearValue(vk::ClearColorValue(1.0f, 0.0f, 1.0f, 1.0f));
    constexpr auto depthClearValue = vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0));


    m_resolveColorImageMemory.getImageMemory().transitImageLayout(
            commandBuffer,
            ImageLayoutTransition{ .oldLayout = vk::ImageLayout::eUndefined,
                                   .newLayout = vk::ImageLayout::eColorAttachmentOptimal });

    const auto colorAttachment = vk::RenderingAttachmentInfo{
        .imageView = m_colorImageMemory.getImageView(),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .resolveMode = vk::ResolveModeFlagBits::eAverage,
        .resolveImageView = m_resolveColorImageMemory.getImageView(),
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

    commandBuffer.beginRendering(renderingInfo);
    for (const auto& pipeline : m_pipelines) {
        pipeline->draw(commandBuffer, m_models);
    }
    commandBuffer.endRendering();


    m_resolveColorImageMemory.getImageMemory().transitImageLayout(
            commandBuffer,
            ImageLayoutTransition{ .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                   .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal });


    //m_swapChain.renderImage(m_resolveColorImageMemory.getImage());

    transitImageLayout(commandBuffer,
                       m_swapChain.getCurrentSwapChainFrame().image,
                       ImageLayoutTransition{ .oldLayout = vk::ImageLayout::eUndefined,
                                              .newLayout = vk::ImageLayout::eColorAttachmentOptimal },
                       1);

    const auto guiColorAttachment = vk::RenderingAttachmentInfo{
        .imageView = m_swapChain.getCurrentSwapChainFrame().imageView,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .resolveMode = vk::ResolveModeFlagBits::eNone,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColorValues,
    };

    const auto guiRenderingInfo = vk::RenderingInfo{
        .renderArea =
                vk::Rect2D{ .offset = vk::Offset2D{ .x = 0, .y = 0 }, .extent = m_swapChain.getSwapChainExtent() },
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &guiColorAttachment,
    };

    commandBuffer.beginRendering(guiRenderingInfo);

    m_gui.start();
    m_gui.draw(commandBuffer);

    commandBuffer.endRendering();

    m_swapChain.submitFrame();
}
}// namespace th