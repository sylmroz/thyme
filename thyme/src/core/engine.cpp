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

    constexpr auto dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    const auto dynamicStateCreateInfo =
            vk::PipelineDynamicStateCreateInfo(vk::PipelineDynamicStateCreateFlagBits(), dynamicStates);
    constexpr auto vertexInputStateCreateInfo =
            vk::PipelineVertexInputStateCreateInfo(vk::PipelineVertexInputStateCreateFlagBits());
    constexpr auto inputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo(
            vk::PipelineInputAssemblyStateCreateFlagBits(), vk::PrimitiveTopology::eTriangleList, vk::True);
    const auto viewport = vk::Viewport(0, 0, extent.width, extent.height, 0, 1);
    const auto scissor = vk::Rect2D(vk::Offset2D(), extent);
    const auto viewportState =
            vk::PipelineViewportStateCreateInfo(vk::PipelineViewportStateCreateFlagBits(), { viewport }, { scissor });
    constexpr auto rasterizer = vk::PipelineRasterizationStateCreateInfo(vk::PipelineRasterizationStateCreateFlagBits(),
                                                                         vk::False,
                                                                         vk::False,
                                                                         vk::PolygonMode::eFill,
                                                                         vk::CullModeFlagBits::eBack,
                                                                         vk::FrontFace::eClockwise,
                                                                         vk::False,
                                                                         0.0f,
                                                                         0.0f,
                                                                         0.0f,
                                                                         1.0f);
    constexpr auto multisampling = vk::PipelineMultisampleStateCreateInfo(vk::PipelineMultisampleStateCreateFlagBits(),
                                                                          vk::SampleCountFlagBits::e1,
                                                                          vk::False,
                                                                          1.0f,
                                                                          nullptr,
                                                                          vk::False,
                                                                          vk::False);
    constexpr auto colorBlendAttachments = vk::PipelineColorBlendAttachmentState(
            vk::False,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                    | vk::ColorComponentFlagBits::eB);
    const auto colorBlendStateCreateInfo =
            vk::PipelineColorBlendStateCreateInfo(vk::PipelineColorBlendStateCreateFlagBits(),
                                                  vk::False,
                                                  vk::LogicOp::eClear,
                                                  1,
                                                  &colorBlendAttachments,
                                                  std::array{ 0.0f, 0.0f, 0.0f, 0.0f });
    const auto pipelineLayout = logicalDevice->createPipelineLayout(vk::PipelineLayoutCreateInfo());

    const auto colorAttachment = vk::AttachmentDescription(vk::AttachmentDescriptionFlagBits(),
                                                           surfaceFormat.format,
                                                           vk::SampleCountFlagBits::e1,
                                                           vk::AttachmentLoadOp::eClear,
                                                           vk::AttachmentStoreOp::eStore,
                                                           vk::AttachmentLoadOp::eDontCare,
                                                           vk::AttachmentStoreOp::eDontCare,
                                                           vk::ImageLayout::eUndefined,
                                                           vk::ImageLayout::ePresentSrcKHR);
    constexpr auto colorAttachmentRef = vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);
    const auto subpassDescription = vk::SubpassDescription(
            vk::SubpassDescriptionFlagBits(), vk::PipelineBindPoint::eGraphics, { colorAttachmentRef });
    const auto renderPass = logicalDevice->createRenderPass(
            vk::RenderPassCreateInfo(vk::RenderPassCreateFlagBits(), { colorAttachment }, { subpassDescription }));

    while (!window.shouldClose()) {
        window.poolEvents();
    }
}
