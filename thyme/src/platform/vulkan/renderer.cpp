#include <thyme/core/logger.hpp>
#include <thyme/platform/vulkan/renderer.hpp>

#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.hpp>

namespace th::vulkan {

VulkanRenderer::VulkanRenderer(const VulkanGlfwWindow& window, const Device& device,
                               const vk::UniqueSurfaceKHR& surface, scene::ModelStorage& modelStorage,
                               scene::Camera& camera) noexcept
    : m_device{ device }, m_window{ window }, m_surface{ surface },
      m_commandPool{ device.logicalDevice->createCommandPoolUnique(
              vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                        device.queueFamilyIndices.graphicFamily.value())) },
      m_swapChainSettings{ device.swapChainSupportDetails.getBestSwapChainSettings() },
      m_swapChainExtent{ m_device.swapChainSupportDetails.getSwapExtent(m_window.getFrameBufferSize()) },
      m_colorImageMemory(ImageMemory(
              device, Resolution{ m_swapChainExtent.width, m_swapChainExtent.height },
              m_swapChainSettings.surfaceFormat.format,
              vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
              vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor, m_device.maxMsaaSamples, 1)),
      m_depthImage(ImageMemory(device, Resolution{ m_swapChainExtent.width, m_swapChainExtent.height },
                               findDepthFormat(device.physicalDevice), vk::ImageUsageFlagBits::eDepthStencilAttachment,
                               vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth,
                               m_device.maxMsaaSamples, 1)),
      m_renderPass{ createRenderPass(device.logicalDevice.get(), m_swapChainSettings.surfaceFormat.format,
                                     findDepthFormat(device.physicalDevice), device.maxMsaaSamples) },
      m_frameDataList{ FrameDataList(device.logicalDevice.get(), m_commandPool.get(), maxFramesInFlight) },
      m_swapChainData{ SwapChainData(m_device, m_swapChainSettings, m_swapChainExtent, m_renderPass.get(),
                                     m_surface.get(), m_colorImageMemory.getImageView().get(),
                                     m_depthImage.getImageView().get()) },
      m_camera{ camera } {
    m_pipelines.emplace_back(
            std::make_unique<ScenePipeline>(device, m_renderPass.get(), m_commandPool.get(), modelStorage, camera));
}

void VulkanRenderer::draw() {
    if (m_window.isMinimalized()) {
        return;
    }
    const auto& [commandBuffer, imageAvailableSemaphore, renderFinishedSemaphore, fence, currentFrame] =
            m_frameDataList.getNext();
    const auto& logicalDevice = m_device.logicalDevice;
    const auto& queueFamilyIndices = m_device.queueFamilyIndices;
    if (logicalDevice->waitForFences({ fence.get() }, vk::True, std::numeric_limits<uint64_t>::max())
        != vk::Result::eSuccess) {
        TH_API_LOG_ERROR("Failed to wait for a complete fence");
        throw std::runtime_error("Failed to wait for a complete fence");
    }
    const auto imageIndexResult = [&] {
        try {
            return logicalDevice->acquireNextImageKHR(
                    m_swapChainData.swapChain.get(), std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore.get());
        } catch (const vk::OutOfDateKHRError&) {
            recreateSwapChain();
        }
        return vk::ResultValue<uint32_t>(vk::Result::eErrorOutOfDateKHR, 0);
    }();

    if (imageIndexResult.result != vk::Result::eSuccess) {
        return;
    }
    const auto imageIndex = imageIndexResult.value;

    logicalDevice->resetFences({ fence.get() });
    commandBuffer->reset();

    commandBuffer->begin(vk::CommandBufferBeginInfo());
    constexpr auto clearColorValues = vk::ClearValue(vk::ClearColorValue(1.0f, 0.0f, 1.0f, 1.0f));
    constexpr auto depthClearValue = vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0));
    constexpr auto clearValues = std::array{ clearColorValues, depthClearValue };
    const auto renderPassBeginInfo =
            vk::RenderPassBeginInfo(m_renderPass.get(),
                                    m_swapChainData.swapChainFrame[imageIndex].frameBuffer.get(),
                                    vk::Rect2D(vk::Offset2D(0, 0), m_swapChainExtent),
                                    clearValues);
    commandBuffer->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

    commandBuffer->setViewport(0,
                               { vk::Viewport(0.0f,
                                              0.0f,
                                              static_cast<float>(m_swapChainExtent.width),
                                              static_cast<float>(m_swapChainExtent.height),
                                              0.0f,
                                              1.0f) });
    commandBuffer->setScissor(0, { vk::Rect2D(vk::Offset2D(0, 0), m_swapChainExtent) });
    bool showDemoWindow = true;
    ImGui::NewFrame();

    ImGui::ShowDemoWindow(&showDemoWindow);
    ImGui::Render();

    for (const auto& pipeline : m_pipelines) {
        pipeline->draw(commandBuffer.get());
    }

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer.get());

    commandBuffer->endRenderPass();
    commandBuffer->end();

    // Update and Render additional Platform Windows
    if (const auto io = ImGui::GetIO(); io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    constexpr vk::PipelineStageFlags f = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const auto submitInfo = vk::SubmitInfo(vk::SubmitInfo(
            { imageAvailableSemaphore.get() }, { f }, { commandBuffer.get() }, { renderFinishedSemaphore.get() }));
    const auto& graphicQueue = logicalDevice->getQueue(m_device.queueFamilyIndices.graphicFamily.value(), 0);
    graphicQueue.submit(submitInfo, fence.get());
    const auto& presentationQueue = logicalDevice->getQueue(queueFamilyIndices.presentFamily.value(), 0);

    try {
        const auto queuePresentResult = presentationQueue.presentKHR(
                vk::PresentInfoKHR({ renderFinishedSemaphore.get() }, { m_swapChainData.swapChain.get() }, { imageIndex }));
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
    m_swapChainSettings = SwapChainSupportDetails(m_device.physicalDevice, m_surface.get()).getBestSwapChainSettings();
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
    m_swapChainData = SwapChainData(m_device,
                                    m_swapChainSettings,
                                    m_swapChainExtent,
                                    m_renderPass.get(),
                                    m_surface.get(),
                                    m_colorImageMemory.getImageView().get(),
                                    m_depthImage.getImageView().get(),
                                    m_swapChainData.swapChain.get());
    m_camera.setResolution(glm::vec2{ resolution.width, resolution.height });
}

}// namespace th::vulkan