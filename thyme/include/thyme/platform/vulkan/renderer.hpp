#pragma once

#include <thyme/platform/glfw_window.hpp>
#include <thyme/platform/vulkan/graphic_pipeline.hpp>
#include <thyme/platform/vulkan/gui.hpp>
#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/platform/vulkan/vulkan_swapchain.hpp>
#include <thyme/renderer/renderer.hpp>
#include <thyme/scene/camera.hpp>
#include <thyme/scene/model.hpp>

#include <vulkan/vulkan.hpp>

namespace th::vulkan {

class VulkanRenderTarget {
public:
    VulkanRenderTarget(vk::ImageView imageView, vk::ImageView resolvedImageView, vk::ImageLayout imageLayout,
                       vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp, vk::ClearValue clearValue);
    VulkanRenderTarget(vk::ImageView imageView, vk::ImageLayout imageLayout, vk::AttachmentLoadOp loadOp,
                       vk::AttachmentStoreOp storeOp, vk::ClearValue clearValue);

    auto getAttachmentInfo() const noexcept -> vk::RenderingAttachmentInfo {
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

class VulkanRenderer final: public renderer::Renderer {
public:
    explicit VulkanRenderer(const VulkanDevice& device, VulkanSwapChain& swapChain, scene::ModelStorage& modelStorage,
                            scene::Camera& camera, Gui& gui, const VulkanGraphicContext& context,
                            VulkanCommandBuffersPool* commandBuffersPool) noexcept;

    void draw() override;

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
    VulkanCommandBuffersPool* m_commandBuffersPool;
    std::vector<std::unique_ptr<GraphicPipeline>> m_pipelines;

    DepthImageMemory m_depthImageMemory;
    ColorImageMemory m_colorImageMemory;
    ColorImageMemory m_resolveColorImageMemory;
};
}// namespace th::vulkan