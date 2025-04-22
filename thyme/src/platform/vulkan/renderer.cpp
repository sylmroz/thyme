#include <thyme/core/logger.hpp>
#include <thyme/platform/vulkan/renderer.hpp>

#include <vulkan/vulkan.hpp>

namespace th::vulkan {

VulkanRenderer::VulkanRenderer(const VulkanGlfwWindow& window, const Device& device,
                               const vk::UniqueSurfaceKHR& surface, scene::ModelStorage& modelStorage,
                               scene::Camera& camera, Gui& gui) noexcept
    : m_device{ device }, m_window{ window }, m_surface{ surface },
      m_commandPool{ device.logicalDevice->createCommandPoolUnique(
              vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                        device.queueFamilyIndices.graphicFamily.value())) },
      m_swapChainSettings{ device.swapChainSupportDetails.getBestSwapChainSettings() },
      m_swapChainExtent{ device.swapChainSupportDetails.getSwapExtent(m_window.getFrameBufferSize()) },
      m_colorImageMemory(ImageMemory(
              device, Resolution{ .width = m_swapChainExtent.width, .height = m_swapChainExtent.height },
              m_swapChainSettings.surfaceFormat.format,
              vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
              vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor, m_device.maxMsaaSamples, 1)),
      m_depthImage(ImageMemory(device, Resolution{ .width = m_swapChainExtent.width, .height = m_swapChainExtent.height },
                               findDepthFormat(device.physicalDevice), vk::ImageUsageFlagBits::eDepthStencilAttachment,
                               vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth,
                               m_device.maxMsaaSamples, 1)),
      m_frameDataList{ FrameDataList(device.logicalDevice.get(), m_commandPool.get(), maxFramesInFlight) },
      m_swapChainData{ SwapChainData(device, m_swapChainSettings, m_swapChainExtent, m_surface.get()) },
      m_camera{ camera }, m_gui{ gui } {
    m_pipelines.emplace_back(std::make_unique<ScenePipeline>(
            device,
            vk::PipelineRenderingCreateInfo(
                    0, { m_swapChainSettings.surfaceFormat.format }, findDepthFormat(device.physicalDevice)),
            m_commandPool.get(),
            modelStorage,
            camera));
}

void VulkanRenderer::draw() {
    if (m_window.isMinimalized()) {
        return;
    }
    const auto& [commandBuffer, imageAvailableSemaphore, renderFinishedSemaphore, fence, currentFrame] =
            m_frameDataList.getNext();
    const auto& logicalDevice = m_device.logicalDevice;
    const auto& queueFamilyIndices = m_device.queueFamilyIndices;
    if (logicalDevice->waitForFences({ fence }, vk::True, std::numeric_limits<uint64_t>::max())
        != vk::Result::eSuccess) {
        TH_API_LOG_ERROR("Failed to wait for a complete fence");
        throw std::runtime_error("Failed to wait for a complete fence");
    }
    const auto imageIndexResult = [&] {
        try {
            return logicalDevice->acquireNextImageKHR(
                    m_swapChainData.getSwapChain(), std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore);
        } catch (const vk::OutOfDateKHRError&) {
            recreateSwapChain();
        }
        return vk::ResultValue<uint32_t>(vk::Result::eErrorOutOfDateKHR, 0);
    }();

    if (imageIndexResult.result != vk::Result::eSuccess) {
        return;
    }
    const auto imageIndex = imageIndexResult.value;

    logicalDevice->resetFences({ fence });
    commandBuffer.reset();

    commandBuffer.begin(vk::CommandBufferBeginInfo());
    constexpr auto clearColorValues = vk::ClearValue(vk::ClearColorValue(1.0f, 0.0f, 1.0f, 1.0f));
    constexpr auto depthClearValue = vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0));

    const auto& [image, imageView] = m_swapChainData.getSwapChainFrame(imageIndex);
    transitImageLayout(commandBuffer, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, 1);

    transitDepthImageLayout(commandBuffer);

    commandBuffer.setViewport(0,
                              { vk::Viewport(0.0f,
                                             0.0f,
                                             static_cast<float>(m_swapChainExtent.width),
                                             static_cast<float>(m_swapChainExtent.height),
                                             0.0f,
                                             1.0f) });
    commandBuffer.setScissor(0, { vk::Rect2D(vk::Offset2D(0, 0), m_swapChainExtent) });

    const auto colorAttachment = vk::RenderingAttachmentInfo(m_colorImageMemory.getImageView(),
                                                             vk::ImageLayout::eColorAttachmentOptimal,
                                                             vk::ResolveModeFlagBits::eAverage,
                                                             imageView,
                                                             vk::ImageLayout::eColorAttachmentOptimal,
                                                             vk::AttachmentLoadOp::eClear,
                                                             vk::AttachmentStoreOp::eStore,
                                                             clearColorValues);

    const auto depthAttachment = vk::RenderingAttachmentInfo(m_depthImage.getImageView(),
                                                             vk::ImageLayout::eDepthAttachmentOptimal,
                                                             vk::ResolveModeFlagBits::eNone,
                                                             {},
                                                             {},
                                                             vk::AttachmentLoadOp::eClear,
                                                             vk::AttachmentStoreOp::eDontCare,
                                                             depthClearValue);

    const auto renderingInfo = vk::RenderingInfo(vk::RenderingFlagBits{},
                                                 vk::Rect2D(vk::Offset2D(0, 0), m_swapChainExtent),
                                                 1,
                                                 0,
                                                 { colorAttachment },
                                                 &depthAttachment);
    commandBuffer.beginRendering(renderingInfo);
    for (const auto& pipeline : m_pipelines) {
        pipeline->draw(commandBuffer);
    }
    m_gui.draw(commandBuffer);

    commandBuffer.endRendering();
    transitImageLayout(
            commandBuffer, image, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, 1);

    commandBuffer.end();

    constexpr vk::PipelineStageFlags f = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const auto submitInfo = vk::SubmitInfo(
            vk::SubmitInfo({ imageAvailableSemaphore }, { f }, { commandBuffer }, { renderFinishedSemaphore }));
    const auto& graphicQueue = logicalDevice->getQueue(queueFamilyIndices.graphicFamily.value(), 0);
    graphicQueue.submit(submitInfo, fence);
    const auto& presentationQueue = logicalDevice->getQueue(queueFamilyIndices.presentFamily.value(), 0);

    try {
        const auto swapChain = m_swapChainData.getSwapChain();
        const auto queuePresentResult = presentationQueue.presentKHR(
                vk::PresentInfoKHR({ renderFinishedSemaphore }, { swapChain }, { imageIndex }));
        if (queuePresentResult == vk::Result::eErrorOutOfDateKHR || queuePresentResult == vk::Result::eSuboptimalKHR
            || m_window.frameBufferResized) {
            m_window.frameBufferResized = false;
            recreateSwapChain();
        }
        if (queuePresentResult != vk::Result::eSuccess) {
            TH_API_LOG_ERROR("Failed to present rendered result!");
            throw std::runtime_error("Failed to present rendered result!");
        }
    } catch (const vk::OutOfDateKHRError&) {
        recreateSwapChain();
    }
}

inline void VulkanRenderer::recreateSwapChain(const Resolution& resolution) {
    m_device.logicalDevice->waitIdle();
    const auto swapChainSupportDetails = SwapChainSupportDetails(m_device.physicalDevice, m_surface.get());
    m_swapChainExtent = swapChainSupportDetails.getSwapExtent(resolution);
    m_swapChainSettings = swapChainSupportDetails.getBestSwapChainSettings();
    m_colorImageMemory =
            ImageMemory(m_device,
                        Resolution{ .width = m_swapChainExtent.width, .height = m_swapChainExtent.height },
                        m_swapChainSettings.surfaceFormat.format,
                        vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
                        vk::MemoryPropertyFlagBits::eDeviceLocal,
                        vk::ImageAspectFlagBits::eColor,
                        m_device.maxMsaaSamples,
                        1);
    m_depthImage = ImageMemory(m_device,
                               Resolution{ m_swapChainExtent.width, m_swapChainExtent.height },
                               findDepthFormat(m_device.physicalDevice),
                               vk::ImageUsageFlagBits::eDepthStencilAttachment,
                               vk::MemoryPropertyFlagBits::eDeviceLocal,
                               vk::ImageAspectFlagBits::eDepth,
                               m_device.maxMsaaSamples,
                               1);
    m_swapChainData = SwapChainData(
            m_device, m_swapChainSettings, m_swapChainExtent, m_surface.get(), m_swapChainData.getSwapChain());
    m_camera.setResolution(glm::vec2{ resolution.width, resolution.height });
}

}// namespace th::vulkan