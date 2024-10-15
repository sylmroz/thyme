module;

#include "thyme/core/logger.hpp"
#include "thyme/pch.hpp"
#include "thyme/platform/vulkan_device_manager.hpp"

#include <filesystem>
#include <ranges>
#include <vector>

module thyme.core.engine;

import thyme.core.utils;
import thyme.core.window;
import thyme.platform.glfw_window;

Thyme::Engine::Engine(const EngineConfig& engineConfig) : m_engineConfig{ engineConfig } {}

void Thyme::Engine::run() {

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

    const auto physicalDevice = physicalDevicesManager.getSelectedDevice();
    const auto logicalDevice = physicalDevice.createLogicalDevice();

    [[maybe_unused]] const auto graphicQueue =
            logicalDevice->getQueue(physicalDevice.queueFamilyIndices.graphicFamily.value(), 0);
    [[maybe_unused]] const auto presentationQueue =
            logicalDevice->getQueue(physicalDevice.queueFamilyIndices.presentFamily.value(), 0);


    const auto& swapChainSupportDetails = physicalDevice.swapChainSupportDetails;
    const auto surfaceFormat = swapChainSupportDetails.getBestSurfaceFormat();
    const auto presetMode = swapChainSupportDetails.getBestPresetMode();
    const auto extent = swapChainSupportDetails.getSwapExtent(window.getFrameBufferSize());

    const auto swapChain = Vulkan::SwapChain(
            Vulkan::SwapChainDetails{ .surfaceFormat = surfaceFormat, .presetMode = presetMode, .extent = extent },
            physicalDevice,
            logicalDevice,
            surface);

    // graphic pipeline
    const auto currentDir = std::filesystem::current_path();
    const auto shaderPath = currentDir / "../../../../thyme/include/thyme/platform/shaders/spv";
    const auto shaderAbsolutePath = std::filesystem::absolute(shaderPath);
    const auto vertShader = readFile(shaderAbsolutePath / "triangle.vert.spv");
    const auto fragShader = readFile(shaderAbsolutePath / "triangle.frag.spv");
    const auto vertexShaderModule = logicalDevice->createShaderModule(vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlagBits(), vertShader.size(), reinterpret_cast<const uint32_t*>(vertShader.data())));
    const auto fragmentShaderModule = logicalDevice->createShaderModule(vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlagBits(), fragShader.size(), reinterpret_cast<const uint32_t*>(fragShader.data())));
    const auto vertexShaderStageInfo = vk::PipelineShaderStageCreateInfo(
            vk::PipelineShaderStageCreateFlagBits(), vk::ShaderStageFlagBits::eVertex, vertexShaderModule, "main");
    const auto fragmentShaderStageInfo = vk::PipelineShaderStageCreateInfo(
            vk::PipelineShaderStageCreateFlagBits(), vk::ShaderStageFlagBits::eFragment, fragmentShaderModule, "main");
    const auto shaderStages = { vertexShaderStageInfo, fragmentShaderStageInfo };

    while (!window.shouldClose()) {
        window.poolEvents();
    }
}
