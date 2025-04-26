#pragma once

#include <thyme/renderer/renderer.hpp>
#include <thyme/platform/glfw_window.hpp>
#include <thyme/platform/vulkan/graphic_pipeline.hpp>
#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/scene/camera.hpp>
#include <thyme/scene/model.hpp>
#include <thyme/platform/vulkan/swapchain.hpp>
#include <thyme/platform/vulkan/gui.hpp>

#include <vulkan/vulkan.hpp>

namespace th::vulkan {

class VulkanRenderer final: public renderer::Renderer {
public:
    explicit VulkanRenderer(const VulkanGlfwWindow& window, const Device& device, const vk::UniqueSurfaceKHR& surface,
                            scene::ModelStorage& modelStorage, scene::Camera& camera, Gui& gui) noexcept;

    void draw() override;

    void windowResized(const Resolution& resolution) {
        recreateSwapChain(resolution);
    }

private:
    inline void recreateSwapChain(const Resolution& resolution);
    
    inline void recreateSwapChain() {
        recreateSwapChain(m_window.getFrameBufferSize());
    }

    inline void transitDepthImageLayout(const vk::CommandBuffer commandBuffer) const {
        transitImageLayout(commandBuffer,
                               m_depthImage.getImage(),
                               ImageLayoutTransition{
                                   .oldLayout =vk::ImageLayout::eUndefined,
                                   .newLayout = vk::ImageLayout::eDepthAttachmentOptimal
                               },
                               ImagePipelineStageTransition {
                                   .oldStage = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
                                   .newStage = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests
                               },
                               ImageAccessFlagsTransition {
                                   .oldAccess = vk::AccessFlags(),
                                   .newAccess = vk::AccessFlagBits::eDepthStencilAttachmentWrite
                               },
                               vk::ImageAspectFlagBits::eDepth,
                               1);
    }

private:
    const Device& m_device;
    const VulkanGlfwWindow& m_window;
    const vk::UniqueSurfaceKHR& m_surface;
    const vk::UniqueCommandPool m_commandPool;
    SwapChainSettings m_swapChainSettings;
    vk::Extent2D m_swapChainExtent;
    ImageMemory m_colorImageMemory;
    ImageMemory m_depthImage;
    FrameDataList m_frameDataList;
    SwapChainData m_swapChainData;
    scene::Camera& m_camera;
    Gui& m_gui;
    std::vector<vk::UniqueCommandBuffer> m_commandBuffers;
    size_t m_commandBufferIndex{ 0 };

    static constexpr uint32_t maxFramesInFlight{ 2 };
    std::vector<std::unique_ptr<GraphicPipeline>> m_pipelines;
};
}// namespace th::vulkan