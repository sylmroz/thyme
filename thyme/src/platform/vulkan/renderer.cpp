#include <thyme/core/logger.hpp>
#include <thyme/platform/vulkan/renderer.hpp>

#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.hpp>

using namespace Thyme::Vulkan;

VulkanRenderer::VulkanRenderer(const VulkanGlfwWindow& window, const Device& device,
                               const vk::UniqueSurfaceKHR& surface) noexcept
    : m_device{ device }, m_window{ window }, m_surface{ surface },
      m_commandPool{ device.logicalDevice->createCommandPoolUnique(
              vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                        device.queueFamilyIndices.graphicFamily.value())) },
      m_swapChainSettings{ device.swapChainSupportDetails.getBestSwapChainSettings() },
      m_swapChainExtent{ m_device.swapChainSupportDetails.getSwapExtent(m_window.getFrameBufferSize()) },
      m_colorImageMemory(createImageMemory(
              device, Resolution{ m_swapChainExtent.width, m_swapChainExtent.height },
              m_swapChainSettings.surfaceFormat.format,
              vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
              vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor, m_device.maxMsaaSamples, 1)),
      m_depthImage(createImageMemory(
              device, Resolution{ m_swapChainExtent.width, m_swapChainExtent.height },
              findDepthFormat(device.physicalDevice), vk::ImageUsageFlagBits::eDepthStencilAttachment,
              vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth, m_device.maxMsaaSamples, 1)),
      m_renderPass{ createRenderPass(device.logicalDevice, m_swapChainSettings.surfaceFormat.format,
                                     findDepthFormat(device.physicalDevice), device.maxMsaaSamples) },
      m_frameDataList{ FrameDataList(device.logicalDevice, m_commandPool, maxFramesInFlight) },
      m_swapChainData{ SwapChainData(m_device, m_swapChainSettings, m_swapChainExtent, m_renderPass, m_surface,
                                     m_colorImageMemory.imageView, m_depthImage.imageView,
                                     m_swapChainData.swapChain.get()) } {
    m_pipelines.emplace_back(std::make_unique<TriangleGraphicPipeline>(device, m_renderPass, m_commandPool));
}

void VulkanRenderer::draw() {
    const auto& [commandBuffer, imageAvailableSemaphore, renderFinishedSemaphore, fence, currentFrame] =
            m_frameDataList.getNext();
    const auto& logicalDevice = m_device.logicalDevice;
    const auto& queueFamilyIndices = m_device.queueFamilyIndices;
    if (logicalDevice->waitForFences({ *fence }, vk::True, std::numeric_limits<uint64_t>::max())
        != vk::Result::eSuccess) {
        TH_API_LOG_ERROR("Failed to wait for a complete fence");
        throw std::runtime_error("Failed to wait for a complete fence");
    }
    const auto imageIndexResult = [&] {
        try {
            return logicalDevice->acquireNextImageKHR(
                    *m_swapChainData.swapChain, std::numeric_limits<uint64_t>::max(), *imageAvailableSemaphore);
        } catch (const vk::OutOfDateKHRError& err) {
            recreateSwapChain();
        }
        return vk::ResultValue<uint32_t>(vk::Result::eErrorOutOfDateKHR, 0);
    }();

    if (imageIndexResult.result != vk::Result::eSuccess) {
        return;
    }
    const auto imageIndex = imageIndexResult.value;

    logicalDevice->resetFences({ *fence });
    commandBuffer->reset();

    commandBuffer->begin(vk::CommandBufferBeginInfo());
    constexpr auto clearColorValues = vk::ClearValue(vk::ClearColorValue(1.0f, 0.0f, 1.0f, 1.0f));
    constexpr auto depthClearValue = vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0));
    constexpr auto clearValues = std::array{ clearColorValues, depthClearValue };
    const auto renderPassBeginInfo = vk::RenderPassBeginInfo(*m_renderPass,
                                                             *m_swapChainData.swapChainFrame[imageIndex].frameBuffer,
                                                             vk::Rect2D(vk::Offset2D(0, 0), m_swapChainExtent),
                                                             clearValues);
    commandBuffer->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

    commandBuffer->setViewport(0,
                               { vk::Viewport(0, 0, m_swapChainExtent.width, m_swapChainExtent.height, 0.0f, 1.0f) });
    commandBuffer->setScissor(0, { vk::Rect2D(vk::Offset2D(0, 0), m_swapChainExtent) });
    bool showDemoWindow = true;
    ImGui::NewFrame();

    ImGui::ShowDemoWindow(&showDemoWindow);
    ImGui::Render();

    for (const auto& pipeline : m_pipelines) {
        pipeline->draw(commandBuffer, m_swapChainExtent, currentFrame);
    }

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer.get());

    commandBuffer->endRenderPass();
    commandBuffer->end();

    // Update and Render additional Platform Windows
    if (const auto io = ImGui::GetIO(); io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    const vk::PipelineStageFlags f = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const auto submitInfo = vk::SubmitInfo(
            vk::SubmitInfo({ *imageAvailableSemaphore }, { f }, { *commandBuffer }, { *renderFinishedSemaphore }));
    const auto& graphicQueue = logicalDevice->getQueue(m_device.queueFamilyIndices.graphicFamily.value(), 0);
    graphicQueue.submit(submitInfo, *fence);
    const auto& presentationQueue = logicalDevice->getQueue(queueFamilyIndices.presentFamily.value(), 0);

    try {
        const auto queuePresentResult = presentationQueue.presentKHR(
                vk::PresentInfoKHR({ *renderFinishedSemaphore }, { *m_swapChainData.swapChain }, { imageIndex }));
        if (queuePresentResult == vk::Result::eErrorOutOfDateKHR || queuePresentResult == vk::Result::eSuboptimalKHR
            || m_window.frameBufferResized) {
            m_window.frameBufferResized = false;
            recreateSwapChain();
        }
        if (queuePresentResult != vk::Result::eSuccess) {
            TH_API_LOG_ERROR("Failed to present rendered result!");
            throw std::runtime_error("Failed to present rendered result!");
        }
    } catch (const vk::OutOfDateKHRError& error) {
        recreateSwapChain();
    }
}
