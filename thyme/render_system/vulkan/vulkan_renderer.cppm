module;

#include <memory>
#include <vector>

export module th.render_system.vulkan:renderer;

import vulkan_hpp;

import th.scene.camera;
import th.scene.model;

import :buffer;
import :command_buffers;
import :device;
import :graphic_pipeline;
import :graphic_context;
import :swap_chain;
import :model;
import :texture;
import :uniform_buffer_object;
import :utils;
import :gui;

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
    explicit VulkanRenderer(const VulkanDevice& device, VulkanSwapChain& swapChain, ModelStorage& modelStorage,
                            Camera& camera, Gui& gui, const VulkanGraphicContext& context,
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
    std::reference_wrapper<Camera> m_camera;

    std::reference_wrapper<ModelStorage> m_modelStorage;
    VulkanUniformBuffer<CameraMatrices> m_cameraMatrices;
};

}// namespace th
