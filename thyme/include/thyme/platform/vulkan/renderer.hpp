#pragma once

#include <thyme/core/logger.hpp>
#include <thyme/renderer/renderer.hpp>
#include <thyme/platform/glfw_window.hpp>
#include <thyme/platform/vulkan/graphic_pipeline.hpp>
#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/scene/camera.hpp>
#include <thyme/scene/model.hpp>

#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.hpp>

namespace th::vulkan {

class VulkanRenderer final: public renderer::Renderer {
public:
    explicit VulkanRenderer(const VulkanGlfwWindow& window, const Device& device, const vk::UniqueSurfaceKHR& surface,
                            scene::ModelStorage& modelStorage, scene::Camera& camera) noexcept;

    void draw() override;

    void windowResized(const Resolution& resolution) {
        recreateSwapChain(resolution);
    }

    // temporary public
    const Device& m_device;
    const VulkanGlfwWindow& m_window;
    const vk::UniqueSurfaceKHR& m_surface;
    const vk::UniqueCommandPool m_commandPool;
    SwapChainSettings m_swapChainSettings;
    vk::Extent2D m_swapChainExtent;
    ImageMemory m_colorImageMemory;
    ImageMemory m_depthImage;
    const vk::UniqueRenderPass m_renderPass;
    FrameDataList m_frameDataList;
    SwapChainData m_swapChainData;
    scene::Camera& m_camera;

    static constexpr uint32_t maxFramesInFlight{ 2 };

private:
    inline void recreateSwapChain(const Resolution& resolution);
    
    inline void recreateSwapChain() {
        recreateSwapChain(m_window.getFrameBufferSize());
    }

private:
    std::vector<std::unique_ptr<GraphicPipeline>> m_pipelines;
};
}// namespace th::vulkan