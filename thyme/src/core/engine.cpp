module;

#include "thyme/core/logger.hpp"
#include "thyme/pch.hpp"
#include "thyme/platform/vulkan_device_manager.hpp"

#include <ranges>
#include <vector>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

module thyme.core.engine;

import thyme.core.event;
import thyme.core.layer;
import thyme.core.layer_stack;
import thyme.core.utils;
import thyme.core.window;
import thyme.platform.glfw_window;
//import thyme.platform.vulkan;

Thyme::Engine::Engine(const EngineConfig& engineConfig, LayerStack<Layer> &layers)
    : m_engineConfig{ engineConfig }, m_layers{ layers } {}

void Thyme::Engine::run() const {
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
            Vulkan::UniqueInstance(Vulkan::UniqueInstanceConfig{ .engineName = m_engineConfig.engineName,
                                                                 .appName = m_engineConfig.appName,
                                                                 .instanceExtension = enabledExtensions });
    const auto surface = window.getSurface(instance.instance);

    const auto devices = Vulkan::getPhysicalDevices(instance.instance, surface);
    const Vulkan::PhysicalDevicesManager physicalDevicesManager(devices);

    const auto& device = physicalDevicesManager.getSelectedDevice();

    Vulkan::VulkanRenderer renderer(window, device, surface);

    // ImGui implementation is temporary.
    // It will have to be adjusted with vulkan and renderer to be more concise together

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    auto& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

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
            Vulkan::createDescriptorPool(device.logicalDevice,
                                         { vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1000),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 1000),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1000),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, 1000),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 1000),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1000),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, 1000),
                                           vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 1000) });
    initInfo.DescriptorPool = descriptorPool.get();
    initInfo.RenderPass = renderer.m_renderPass.get();
    initInfo.Subpass = 0;
    initInfo.MinImageCount = Vulkan::VulkanRenderer::maxFramesInFlight;
    initInfo.ImageCount = Vulkan::VulkanRenderer::maxFramesInFlight;
    initInfo.MSAASamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = [](auto vkResult) {
        if (vkResult == VK_SUCCESS) {
            return;
        }
        TH_API_LOG_INFO("CheckVkResultFn: {}", static_cast<int>(vkResult));
    };
    ImGui_ImplVulkan_Init(&initInfo);

    // Vulkan::singleTimeCommand(
    //         device.logicalDevice,
    //         renderer.m_commandPool,
    //         device.logicalDevice->getQueue(device.queueFamilyIndices.graphicFamily.value(), 0),
    //         [](const vk::UniqueCommandBuffer& commandBuffer) { ImGui_ImplVulkan_CreateFontsTexture(); });


    while (!window.shouldClose()) {
        window.poolEvents();

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        for (const auto& layer : m_layers) {
            layer->draw();
        }
        renderer.draw();
    }

    device.logicalDevice->waitIdle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}