export module th.render_system.vulkan:renderer;

import std;

import vulkan;

import th.scene.camera;
import th.scene.model;
import th.core.logger;

import :buffer;
import :command_buffers;
import :device;
import :graphic_pipeline;
import :graphic_context;
import :swapchain;
import :model;
import :texture;
import :uniform_buffer_object;
import :utils;
import :gui;

namespace th {

export class RenderTarget {
public:
    RenderTarget(vk::ImageView image_view, vk::ImageView resolved_image_view, vk::ImageLayout image_layout,
                 vk::AttachmentLoadOp load_op, vk::AttachmentStoreOp store_op, vk::ClearValue clear_value);
    RenderTarget(vk::ImageView image_view, vk::ImageLayout image_layout, vk::AttachmentLoadOp load_op,
                 vk::AttachmentStoreOp store_op, vk::ClearValue clear_value);

    [[nodiscard]] auto getAttachmentInfo() const noexcept -> vk::RenderingAttachmentInfo {
        return m_attachmentInfo;
    }

private:
    vk::RenderingAttachmentInfo m_attachmentInfo;
};

// class VulkanRenderingInfo {
// public:
//     VulkanRenderingInfo(VulkanSwapChain* swapChain, vk::SampleCountFlagBits samples);
//
//     vk::RenderPassBeginInfo getRenderingBeginInfo() const noexcept;
// private:
//     VulkanSwapChain* m_swapChain;
//     vk::SampleCountFlagBits m_samples;
//     std::optional<ColorImageMemory> m_colorMemory;
//     std::optional<DepthImageMemory> m_depthMemory;
// };
export class VulkanRenderer {
public:
    explicit VulkanRenderer(const VulkanDeviceRAII& device, VulkanSwapchain& swapchain, ModelStorage& model_storage,
                            Camera& camera, Gui& gui, const VulkanGraphicContext& context,
                            VulkanCommandBuffersPool& command_buffers_pool, Logger& logger) noexcept;

    void draw(const VulkanDeviceRAII& device);

private:
    void transitDepthImageLayout(const vk::CommandBuffer command_buffer) const {
        transitImageLayout(command_buffer,
                           m_depth_image_memory.getImage(),
                           ImageLayoutTransition{ .oldLayout = vk::ImageLayout::eUndefined,
                                                  .newLayout = vk::ImageLayout::eDepthAttachmentOptimal },
                           ImagePipelineStageTransition{ .oldStage = vk::PipelineStageFlagBits::eEarlyFragmentTests
                                                                     | vk::PipelineStageFlagBits::eLateFragmentTests,
                                                         .newStage = vk::PipelineStageFlagBits::eEarlyFragmentTests
                                                                     | vk::PipelineStageFlagBits::eLateFragmentTests },
                           ImageAccessFlagsTransition{ .oldAccess = vk::AccessFlags(),
                                                       .newAccess = vk::AccessFlagBits::eDepthStencilAttachmentWrite },
                           vk::ImageAspectFlagBits::eDepth,
                           1);
    }


    Gui& m_gui;
    VulkanSwapchain& m_swapchain;
    VulkanCommandBuffersPool& m_command_buffers_pool;
    std::vector<std::unique_ptr<VulkanGraphicPipeline>> m_pipelines;

    VulkanDepthImageMemory m_depth_image_memory;
    VulkanColorImageMemory m_color_image_memory;
    VulkanColorImageMemory m_resolve_color_image_memory;

    std::vector<VulkanModel> m_models;
    std::reference_wrapper<Camera> m_camera;

    std::reference_wrapper<ModelStorage> m_model_storage;
    VulkanUniformBuffer<CameraMatrices> m_camera_matrices;
};

}// namespace th
