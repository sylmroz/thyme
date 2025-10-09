module;

module th.render_system.vulkan;

namespace th {

RenderTarget::RenderTarget(const vk::ImageView image_view, const vk::ImageView resolved_image_view,
                           const vk::ImageLayout image_layout, const vk::AttachmentLoadOp load_op,
                           const vk::AttachmentStoreOp store_op, const vk::ClearValue clear_value) {
    m_attachmentInfo = vk::RenderingAttachmentInfo{
        .imageView = image_view,
        .imageLayout = image_layout,
        .resolveMode = vk::ResolveModeFlagBits::eAverage,
        .resolveImageView = resolved_image_view,
        .resolveImageLayout = image_layout,
        .loadOp = load_op,
        .storeOp = store_op,
        .clearValue = clear_value,
    };
}

RenderTarget::RenderTarget(const vk::ImageView image_view, const vk::ImageLayout image_layout,
                           const vk::AttachmentLoadOp load_op, const vk::AttachmentStoreOp store_op,
                           const vk::ClearValue clear_value) {
    m_attachmentInfo = vk::RenderingAttachmentInfo{
        .imageView = image_view,
        .imageLayout = image_layout,
        .resolveMode = vk::ResolveModeFlagBits::eNone,
        .loadOp = load_op,
        .storeOp = store_op,
        .clearValue = clear_value,
    };
}

// VulkanRenderingInfo::VulkanRenderingInfo(VulkanSwapChain* swapChain, const vk::SampleCountFlagBits samples)
//     : m_swapChain{ swapChain }, m_samples{ samples } {
//     m_colorMemory = ColorImageMemory()
// }

void updateUBO(const Camera& camera, const VulkanUniformBuffer<CameraMatrices>& camera_matrices,
               std::vector<VulkanModel>& models, ModelStorage& model_storage) {
    camera_matrices.update(
            CameraMatrices{ .view = camera.getViewMatrix(), .projection = camera.getProjectionMatrix() });
    for (auto [vlkModel, model] : std::views::zip(models, model_storage)) {
        model.animate();
        vlkModel.getUniformBufferObject().update(model.transformation.getTransformMatrix());
    }
}

VulkanRenderer::VulkanRenderer(const VulkanDevice& device, VulkanSwapchain& swapchain, ModelStorage& model_storage, Camera& camera,
                   Gui& gui, const VulkanGraphicContext& context, VulkanCommandBuffersPool& command_buffers_pool) noexcept
    : m_gui{ gui }, m_swapchain{ swapchain }, m_command_buffers_pool{ command_buffers_pool },
      m_depth_image_memory{ device, swapchain.getSwapchainExtent(), context.depth_format, device.max_msaa_samples },
      m_color_image_memory{ device, swapchain.getSwapchainExtent(), context.color_format, device.max_msaa_samples },
      m_resolve_color_image_memory{ device,
                                 swapchain.getSwapchainExtent(),
                                 context.color_format,
                                 vk::SampleCountFlagBits::e1 },
      m_camera{ camera }, m_model_storage{ model_storage }, m_camera_matrices{ device } {

    for (const auto& model : model_storage) {
        m_models.emplace_back(model, device);
    }
    m_pipelines.emplace_back(std::make_unique<VulkanScenePipeline>(
            device,
            vk::PipelineRenderingCreateInfo{ .viewMask = 0,
                                             .colorAttachmentCount = 1,
                                             .pColorAttachmentFormats = &context.color_format,
                                             .depthAttachmentFormat = context.depth_format },
            m_models,
            m_camera_matrices));
}

void VulkanRenderer::draw() {
    if (!m_swapchain.prepareFrame()) {
        return;
    }
    updateUBO(m_camera, m_camera_matrices, m_models, m_model_storage);
    m_color_image_memory.resize(m_swapchain.getSwapchainExtent());
    m_resolve_color_image_memory.resize(m_swapchain.getSwapchainExtent());
    m_depth_image_memory.resize(m_swapchain.getSwapchainExtent());

    const auto command_buffer = m_command_buffers_pool.get().getBuffer();
    setCommandBufferFrameSize(command_buffer, m_swapchain.getSwapchainExtent());
    constexpr auto clear_color_values = vk::ClearValue(vk::ClearColorValue(1.0f, 0.0f, 1.0f, 1.0f));
    constexpr auto depth_clear_value = vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0));


    m_resolve_color_image_memory.transitImageLayout(
            command_buffer,
            ImageLayoutTransition{ .oldLayout = vk::ImageLayout::eUndefined,
                                   .newLayout = vk::ImageLayout::eColorAttachmentOptimal });

    const auto color_attachment = vk::RenderingAttachmentInfo{
        .imageView = m_color_image_memory.getImageView(),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .resolveMode = vk::ResolveModeFlagBits::eAverage,
        .resolveImageView = m_resolve_color_image_memory.getImageView(),
        .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clear_color_values,
    };

    const auto depth_attachment = vk::RenderingAttachmentInfo{
        .imageView = m_depth_image_memory.getImageView(),
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .resolveMode = vk::ResolveModeFlagBits::eNone,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = depth_clear_value,
    };

    const auto rendering_info = vk::RenderingInfo{
        .renderArea =
                vk::Rect2D{ .offset = vk::Offset2D{ .x = 0, .y = 0 }, .extent = m_swapchain.getSwapchainExtent() },
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = &depth_attachment,
    };

    command_buffer.beginRendering(rendering_info);
    for (const auto& pipeline : m_pipelines) {
        pipeline->draw(command_buffer, m_models);
    }
    command_buffer.endRendering();

    m_resolve_color_image_memory.transitImageLayout(
            command_buffer,
            ImageLayoutTransition{ .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                   .newLayout = vk::ImageLayout::eTransferSrcOptimal });


    m_swapchain.renderImage(m_resolve_color_image_memory.getImage());
    transitImageLayout(command_buffer,
                       m_swapchain.getCurrentSwapchainFrame().image,
                       ImageLayoutTransition{ .oldLayout = vk::ImageLayout::eTransferDstOptimal,
                                              .newLayout = vk::ImageLayout::eColorAttachmentOptimal },
                       1);

    const auto gui_color_attachment = vk::RenderingAttachmentInfo{
        .imageView = m_swapchain.getCurrentSwapchainFrame().image_view,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .resolveMode = vk::ResolveModeFlagBits::eNone,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
    };

    const auto gui_rendering_info = vk::RenderingInfo{
        .renderArea =
                vk::Rect2D{ .offset = vk::Offset2D{ .x = 0, .y = 0 }, .extent = m_swapchain.getSwapchainExtent() },
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &gui_color_attachment,
    };

    command_buffer.beginRendering(gui_rendering_info);

    m_gui.start();
    m_gui.draw(command_buffer);

    command_buffer.endRendering();

    m_swapchain.submitFrame();
}
}