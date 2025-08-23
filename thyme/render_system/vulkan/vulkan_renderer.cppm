module;

#include <memory>
#include <vector>

export module th.render_system.vulkan.renderer;

import vulkan_hpp;

import th.render_system.vulkan.buffer;
import th.render_system.vulkan.command_buffers;
import th.render_system.vulkan.device;
import th.render_system.vulkan.graphic_pipeline;
import th.render_system.vulkan.graphic_context;
import th.render_system.vulkan.swap_chain;
import th.render_system.vulkan.model;
import th.render_system.vulkan.texture;
import th.render_system.vulkan.uniform_buffer_object;
import th.render_system.vulkan.utils;

import th.render_system.vulkan.gui;

import th.scene.camera;
import th.scene.model;


namespace th {

export class RenderTarget {
public:
    RenderTarget(vk::ImageView imageView, vk::ImageView resolvedImageView, vk::ImageLayout imageLayout,
                 vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp, vk::ClearValue clearValue);
    RenderTarget(vk::ImageView imageView, vk::ImageLayout imageLayout, vk::AttachmentLoadOp loadOp,
                 vk::AttachmentStoreOp storeOp, vk::ClearValue clearValue);

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
    explicit VulkanRenderer(const VulkanDevice& device, VulkanSwapChain& swapChain, scene::ModelStorage& modelStorage,
                            scene::Camera& camera, Gui& gui, const VulkanGraphicContext& context,
                            VulkanCommandBuffersPool& commandBuffersPool) noexcept;

    void draw();

private:
    void transitDepthImageLayout(const vk::CommandBuffer commandBuffer) const {
        transitImageLayout(commandBuffer,
                           m_depthImageMemory.getImage(),
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
    VulkanSwapChain& m_swapChain;
    VulkanCommandBuffersPool& m_commandBuffersPool;
    std::vector<std::unique_ptr<VulkanGraphicPipeline>> m_pipelines;

    VulkanDepthImageMemory m_depthImageMemory;
    VulkanColorImageMemory m_colorImageMemory;
    VulkanColorImageMemory m_resolveColorImageMemory;

    std::vector<VulkanModel> m_models;
    std::reference_wrapper<scene::Camera> m_camera;

    std::reference_wrapper<scene::ModelStorage> m_modelStorage;
    VulkanUniformBuffer<scene::CameraMatrices> m_cameraMatrices;
};

}// namespace th
