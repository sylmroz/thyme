module;

#include "thyme/core/logger.hpp"
#include "thyme/pch.hpp"
#include "thyme/platform/vulkan_device_manager.hpp"

#include <thyme/platform/vulkan/graphic_pipeline.hpp>

#include <filesystem>
#include <ranges>
#include <vector>

module thyme.core.engine;

import thyme.core.utils;
import thyme.core.window;
import thyme.platform.glfw_window;

Thyme::Engine::Engine(const EngineConfig& engineConfig) : m_engineConfig{ engineConfig } {}

void Thyme::Engine::run() const {

    TH_API_LOG_INFO("Start {} engine", m_engineConfig.engineName);

    VulkanGlfwWindow window(WindowConfig{ m_engineConfig });

    const auto glfwExtensions = VulkanGlfwWindow::getRequiredInstanceExtensions();
    // clang-format off
    const std::vector<const char*> enabledExtensions = glfwExtensions
            | std::views::transform([](auto const& layerName) { return layerName.c_str(); })
            | std::ranges::to<std::vector<const char*>>();
    // clang-format on

    const auto instance =
            Vulkan::UniqueInstance(Vulkan::UniqueInstanceConfig{ .engineName = m_engineConfig.engineName,
                                                                 .appName = m_engineConfig.appName,
                                                                 .instanceExtension = enabledExtensions });
    const auto surface = window.getSurface(instance.instance);

    const auto devices = Vulkan::getPhysicalDevices(instance.instance, surface);
    const Vulkan::PhysicalDevicesManager physicalDevicesManager(devices);

    const auto device = physicalDevicesManager.getSelectedDevice();
    const auto& logicalDevice = device.logicalDevice;

    const auto& swapChainSupportDetails = device.swapChainSupportDetails;
    const auto swapChainSettings = swapChainSupportDetails.getBestSwapChainSettings(window.getFrameBufferSize());
    const auto surfaceFormat = swapChainSettings.surfaceFormat;
    auto extent = swapChainSettings.extent;

    const auto renderPass = Vulkan::createRenderPass(logicalDevice, surfaceFormat.format);

    auto renderTriangle = Vulkan::TriangleGraphicPipeline(device.logicalDevice, renderPass);

    auto swapChain = Vulkan::SwapChainData(swapChainSettings, device, renderPass, surface);

    const auto commandPool = logicalDevice->createCommandPoolUnique(vk::CommandPoolCreateInfo(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer, device.queueFamilyIndices.graphicFamily.value()));

    constexpr uint32_t maxFrames{ 2 };
    const auto frameDatas = Vulkan::createFrameDatas(device.logicalDevice, commandPool, maxFrames);

    auto getNextFrameIndex = [frameIndex = 0, maxFrames]() mutable {
        const auto currentFrameIndex = frameIndex;
        frameIndex = (frameIndex + 1) % maxFrames;
        return currentFrameIndex;
    };

    auto recreateSwapChain = [&] {
        logicalDevice->waitIdle();

        extent = swapChainSupportDetails.getSwapExtent(window.getFrameBufferSize());
        swapChain = Vulkan::SwapChainData(swapChainSettings, device, renderPass, surface, *swapChain.swapChain);
    };

    const auto drawFrame = [&] {
        const auto frameIndex = getNextFrameIndex();
        const auto& [commandBuffer, imageAvailableSemaphore, renderFinishedSemaphore, fence] = frameDatas[frameIndex];

        if (logicalDevice->waitForFences({ *fence }, vk::True, std::numeric_limits<uint64_t>::max())
            != vk::Result::eSuccess) {
            TH_API_LOG_ERROR("Failed to wait for a complete fence")
            throw std::runtime_error("Failed to wait for a complete fence");
        }
        const auto imageIndexResult = [&] {
            try {
                return logicalDevice->acquireNextImageKHR(
                        *swapChain.swapChain, std::numeric_limits<uint64_t>::max(), *imageAvailableSemaphore);
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
        constexpr auto clearColorValues = vk::ClearColorValue(1.0f, 0.0f, 1.0f, 1.0f);
        constexpr auto clearValues = vk::ClearValue(clearColorValues);
        const auto renderPassBeginInfo = vk::RenderPassBeginInfo(*renderPass,
                                                                 *swapChain.frameBuffers[imageIndex],
                                                                 vk::Rect2D(vk::Offset2D(0, 0), extent),
                                                                 { clearValues });
        commandBuffer->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

        commandBuffer->setViewport(0, { vk::Viewport(0, 0, extent.width, extent.height, 0.0f, 1.0f) });
        commandBuffer->setScissor(0, { vk::Rect2D(vk::Offset2D(0, 0), extent) });

        renderTriangle.draw(commandBuffer);

        commandBuffer->endRenderPass();
        commandBuffer->end();

        const vk::PipelineStageFlags f = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        const auto submitInfo = vk::SubmitInfo(
                vk::SubmitInfo({ *imageAvailableSemaphore }, { f }, { *commandBuffer }, { *renderFinishedSemaphore }));
        const auto& graphicQueue = logicalDevice->getQueue(device.queueFamilyIndices.graphicFamily.value(), 0);
        graphicQueue.submit(submitInfo, *fence);
        const auto& presentationQueue = logicalDevice->getQueue(device.queueFamilyIndices.presentFamily.value(), 0);

        try {
            const auto queuePresentResult = presentationQueue.presentKHR(
                    vk::PresentInfoKHR({ *renderFinishedSemaphore }, { *swapChain.swapChain }, { imageIndex }));
            if (queuePresentResult == vk::Result::eErrorOutOfDateKHR || queuePresentResult == vk::Result::eSuboptimalKHR
                || window.frameBufferResized) {
                window.frameBufferResized = false;
                recreateSwapChain();
            }
            if (queuePresentResult != vk::Result::eSuccess) {
                TH_API_LOG_ERROR("Failed to present rendered result!");
                throw std::runtime_error("Failed to present rendered result!");
            }
        } catch (const vk::OutOfDateKHRError& error) {
            recreateSwapChain();
        }
    };

    while (!window.shouldClose()) {
        window.poolEvents();
        drawFrame();
    }

    logicalDevice->waitIdle();
}
