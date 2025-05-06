#pragma once

#include "vulkan_graphic_context.hpp"
#include "vulkan_texture.hpp"


#include <vulkan/vulkan.hpp>

#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/platform/vulkan/vulkan_device.hpp>
#include <thyme/renderer/swapchain.hpp>

namespace th::vulkan {

struct SwapChainFrame {
    vk::Image image;
    vk::ImageView imageView;
};

class SwapChainFrames {
public:
    explicit SwapChainFrames(vk::Device device, vk::SwapchainKHR swapChain, vk::Format format);

    [[nodiscard]] std::size_t getSwapChainFramesCount() const noexcept {
        return m_images.size();
    }

    [[nodiscard]] auto getSwapChainFrame(const uint32_t index) noexcept -> SwapChainFrame {
        return getSwapChainFrameInternal(index);
    }
    [[nodiscard]] auto getSwapChainFrame(const uint32_t index) const noexcept -> SwapChainFrame {
        return getSwapChainFrameInternal(index);
    }

private:
    [[nodiscard]] auto getSwapChainFrameInternal(const uint32_t index) const -> SwapChainFrame {
        if (index >= m_images.size()) {
            constexpr auto message = "SwapChainFrames index out of range";
            TH_API_LOG_ERROR(message);
            throw std::out_of_range(message);
        }
        return SwapChainFrame{
            .image = m_images[index],
            .imageView = m_imageViews[index].get(),
        };
    }

private:
    std::vector<vk::Image> m_images;
    std::vector<vk::UniqueImageView> m_imageViews;
};

class SwapChainData {
public:
    explicit SwapChainData(const VulkanDevice& device, const SwapChainSettings& swapChainSettings,
                           const vk::Extent2D swapChainExtent, const vk::SurfaceKHR surface,
                           const vk::SwapchainKHR oldSwapChain = {})
        : m_swapChain{ createSwapChain(device, swapChainSettings, swapChainExtent, surface, oldSwapChain) },
          m_swapChainFrames{ device.logicalDevice, m_swapChain.get(), swapChainSettings.surfaceFormat.format } {}

    [[nodiscard]] auto getSwapChain() noexcept -> vk::SwapchainKHR {
        return m_swapChain.get();
    }

    [[nodiscard]] auto getSwapChain() const noexcept -> vk::SwapchainKHR {
        return m_swapChain.get();
    }

    [[nodiscard]] std::size_t getSwapChainFramesCount() const noexcept {
        return m_swapChainFrames.getSwapChainFramesCount();
    }

    [[nodiscard]] auto getSwapChainFrame(const uint32_t index) noexcept -> SwapChainFrame {
        return m_swapChainFrames.getSwapChainFrame(index);
    }

    [[nodiscard]] auto getSwapChainFrame(const uint32_t index) const noexcept -> SwapChainFrame {
        return m_swapChainFrames.getSwapChainFrame(index);
    }

private:
    vk::UniqueSwapchainKHR m_swapChain;
    SwapChainFrames m_swapChainFrames;

private:
    static [[nodiscard]] auto createSwapChain(const VulkanDevice& device, SwapChainSettings swapChainSettings,
                                              vk::Extent2D swapChainExtent, vk::SurfaceKHR surface,
                                              vk::SwapchainKHR oldSwapChain) -> vk::UniqueSwapchainKHR;
};

class VulkanSwapChain final: public renderer::SwapChain {
public:
    VulkanSwapChain(const VulkanDevice& device, vk::SurfaceKHR surface, const VulkanGraphicContext& context,
                    vk::Extent2D swapChainExtent);
    void frameResized(vk::Extent2D resolution);
    bool prepareFrame() override;
    void prepareRenderMode(vk::CommandBuffer commandBuffer);
    void preparePresentMode(vk::CommandBuffer commandBuffer);
    void renderGraphic(vk::CommandBuffer commandBuffer);
    void submitFrame() override;

private:
    bool hasResized() const;
    void recreateSwapChain();
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


private:
    uint32_t m_currentImageIndex{ 0 };
    vk::SurfaceKHR m_surface;
    vk::Extent2D m_swapChainExtent;
    vk::Extent2D m_fallBackExtent{};
    VulkanGraphicContext m_context;
    VulkanDevice m_device;
    SwapChainData m_swapChainData;
    DepthImageMemory m_depthImageMemory;
    ColorImageMemory m_colorImageMemory;
    FrameDataList m_frameDataList;
    FrameData m_currentFrameData;
};

}// namespace th::vulkan
