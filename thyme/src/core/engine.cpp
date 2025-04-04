#include <thyme/core/event.hpp>
#include <thyme/core/logger.hpp>
#include <thyme/core/utils.hpp>
#include <thyme/core/window.hpp>
#include <thyme/platform/glfw_window.hpp>
#include <thyme/platform/vulkan/renderer.hpp>
#include <thyme/platform/vulkan/vulkan_layer.hpp>
#include <thyme/platform/vulkan_device_manager.hpp>

#include <ranges>
#include <vector>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace th {

Engine::Engine(const EngineConfig& engineConfig, vulkan::VulkanLayerStack& layers, scene::ModelStorage& modelStorage)
    : m_engineConfig{ engineConfig },
      m_camera{ scene::CameraArguments{ .fov = 45.0f,
                                        .zNear = 0.1f,
                                        .zFar = 100.0f,
                                        .resolution = { engineConfig.width, engineConfig.height },
                                        .eye = { 2.0f, 2.0f, 2.0f },
                                        .center = { 0.0f, 0.0f, 0.0f },
                                        .up = { 0.0f, 0.0f, 1.0f } } },
      m_layers{ layers }, m_modelStorage{ modelStorage } {
}

void Engine::run() {
    TH_API_LOG_INFO("Start {} engine", m_engineConfig.engineName);

    EventSubject windowEvents;
    windowEvents.subscribe([&layers = m_layers](const Event& event) {
        for (const auto layer : layers) {
            layer->onEvent(event);
        }
    });
    VulkanGlfwWindow window(WindowConfig{ m_engineConfig, windowEvents });

    const auto glfwExtensions = VulkanGlfwWindow::getRequiredInstanceExtensions();
    // clang-format off
    const std::vector<const char*> enabledExtensions = glfwExtensions
            | std::views::transform([](auto const& layerName) { return layerName.c_str(); })
            | std::ranges::to<std::vector<const char*>>();
    // clang-format on

    const auto instance =
            vulkan::UniqueInstance(vulkan::UniqueInstanceConfig{ .engineName = m_engineConfig.engineName,
                                                                 .appName = m_engineConfig.appName,
                                                                 .instanceExtension = enabledExtensions });
    const auto surface = window.getSurface(instance.instance);

    const auto devices = vulkan::getPhysicalDevices(instance.instance, surface);
    const vulkan::PhysicalDevicesManager physicalDevicesManager(devices);

    const auto& device = physicalDevicesManager.getSelectedDevice();

    vulkan::VulkanRenderer renderer(window, device, surface, m_modelStorage, m_camera);

    // ImGui implementation is temporary.
    // It will have to be adjusted with vulkan and renderer to be more concise together
    ImGui_ImplGlfw_InitForVulkan(window.getHandler().get(), true);
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = instance.instance.get();
    initInfo.PhysicalDevice = device.physicalDevice;
    initInfo.Device = device.logicalDevice.get();
    initInfo.QueueFamily = device.queueFamilyIndices.graphicFamily.value();
    initInfo.Queue = device.logicalDevice->getQueue(device.queueFamilyIndices.graphicFamily.value(), 0);
    const auto pipelineCache = device.logicalDevice->createPipelineCacheUnique(vk::PipelineCacheCreateInfo());
    initInfo.PipelineCache = pipelineCache.get();
    const auto descriptorPool =
            vulkan::createDescriptorPool(device.logicalDevice,
                                         { vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 2),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 2),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 2),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 2),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, 2),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 2),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 2),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 2),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 2),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, 2),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 2) });
    initInfo.DescriptorPool = descriptorPool.get();
    initInfo.RenderPass = renderer.m_renderPass.get();
    initInfo.Subpass = 0;
    initInfo.MinImageCount = vulkan::VulkanRenderer::maxFramesInFlight;
    initInfo.ImageCount = vulkan::VulkanRenderer::maxFramesInFlight;
    initInfo.MSAASamples = static_cast<VkSampleCountFlagBits>(device.maxMsaaSamples);
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = [](const auto vkResult) {
        if (vkResult == VK_SUCCESS) {
            return;
        }
        TH_API_LOG_INFO("CheckVkResultFn: {}", static_cast<int>(vkResult));
    };
    ImGui_ImplVulkan_Init(&initInfo);

    while (!window.shouldClose()) {
        window.poolEvents();

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        for (const auto& layer : m_layers) {
            // layer->draw();
        }
        renderer.draw();
    }

    device.logicalDevice->waitIdle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}

}// namespace th