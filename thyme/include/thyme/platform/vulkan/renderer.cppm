module;

#include <thyme/core/logger.hpp>

#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.hpp>

export module thyme.platform.vulkan:renderer;
import :graphic_pipeline;
import :utils;

import thyme.core.renderer;
import thyme.platform.glfw_window;

export namespace Thyme::Vulkan {

class VulkanRenderer final: public Renderer {
public:
    explicit VulkanRenderer(const VulkanGlfwWindow& window, const Device& device, const vk::UniqueSurfaceKHR& surface,
                            const vk::UniqueSampler& sampler) noexcept;

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
    ImageMemory m_depthImage;
    const vk::UniqueRenderPass m_renderPass;
    FrameDataList m_frameDataList;
    SwapChainData m_swapChainData;

    static constexpr uint32_t maxFramesInFlight{ 2 };

private:
    inline void recreateSwapChain(const Resolution& resolution) {
        m_device.logicalDevice->waitIdle();
        const auto swapChainSupportDetails = SwapChainSupportDetails(m_device.physicalDevice, m_surface);
        m_swapChainExtent = swapChainSupportDetails.getSwapExtent(resolution);
        m_swapChainSettings = SwapChainSupportDetails(m_device.physicalDevice, m_surface).getBestSwapChainSettings();
        m_depthImage = createImageMemory(m_device, Resolution{ m_swapChainExtent.width, m_swapChainExtent.height },
                                         findDepthFormat(m_device.physicalDevice),
                                         vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                         vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth);
        m_swapChainData = SwapChainData(m_device,
                                        m_swapChainSettings,
                                        m_swapChainExtent,
                                        m_renderPass,
                                        m_surface,
                                        m_depthImage.imageView,
                                        m_swapChainData.swapChain.get());
    }
    inline void recreateSwapChain() {
        recreateSwapChain(m_window.getFrameBufferSize());
    }

private:
    std::vector<std::unique_ptr<GraphicPipeline>> m_pipelines;
};
}// namespace Thyme::Vulkan