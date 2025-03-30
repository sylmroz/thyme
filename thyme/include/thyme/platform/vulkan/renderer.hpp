#pragma once

#include <thyme/core/logger.hpp>
#include <thyme/core/renderer.hpp>
#include <thyme/platform/glfw_window.hpp>
#include <thyme/platform/vulkan/graphic_pipeline.hpp>
#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/scene/camera.hpp>
#include <thyme/scene/model.hpp>

#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.hpp>

namespace th::vulkan {

class VulkanRenderer final: public Renderer {
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
    inline void recreateSwapChain(const Resolution& resolution) {
        m_device.logicalDevice->waitIdle();
        const auto swapChainSupportDetails = SwapChainSupportDetails(m_device.physicalDevice, m_surface);
        m_swapChainExtent = swapChainSupportDetails.getSwapExtent(resolution);
        m_swapChainSettings = SwapChainSupportDetails(m_device.physicalDevice, m_surface).getBestSwapChainSettings();
        m_colorImageMemory = createImageMemory(m_device,
                                               Resolution{ m_swapChainExtent.width, m_swapChainExtent.height },
                                               m_swapChainSettings.surfaceFormat.format,
                                               vk::ImageUsageFlagBits::eTransientAttachment
                                                       | vk::ImageUsageFlagBits::eColorAttachment,
                                               vk::MemoryPropertyFlagBits::eDeviceLocal,
                                               vk::ImageAspectFlagBits::eColor,
                                               m_device.maxMsaaSamples,
                                               1);
        m_depthImage = createImageMemory(m_device,
                                         Resolution{ m_swapChainExtent.width, m_swapChainExtent.height },
                                         findDepthFormat(m_device.physicalDevice),
                                         vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                         vk::MemoryPropertyFlagBits::eDeviceLocal,
                                         vk::ImageAspectFlagBits::eDepth,
                                         m_device.maxMsaaSamples,
                                         1);
        m_swapChainData = SwapChainData(m_device,
                                        m_swapChainSettings,
                                        m_swapChainExtent,
                                        m_renderPass,
                                        m_surface,
                                        m_colorImageMemory.imageView,
                                        m_depthImage.imageView,
                                        m_swapChainData.swapChain.get());
        m_camera.setResolution(glm::vec2{ resolution.width, resolution.height });
    }
    inline void recreateSwapChain() {
        recreateSwapChain(m_window.getFrameBufferSize());
    }

private:
    std::vector<std::unique_ptr<GraphicPipeline>> m_pipelines;
};
}// namespace th::vulkan