module;

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <spdlog/spdlog.h>

export module th.render_system.vulkan:gui;

import :device;
import :graphic_context;
import :utils;

import th.platform.glfw.glfw_window;
import th.core.logger;

import vulkan_hpp;

namespace th {

export class Gui final {
public:
    explicit Gui(const VulkanDevice& device, const VulkanGlfwWindow& window, const VulkanGraphicContext& context,
                 vk::Instance instance);

    Gui(Gui&& other) noexcept = delete;
    auto operator=(Gui&& other) noexcept -> Gui& = delete;
    Gui(const Gui& other) = delete;
    auto operator=(const Gui& other) -> Gui& = delete;

    void draw(vk::CommandBuffer commandBuffer);

    void start() const;

    ~Gui() noexcept;

private:
    vk::UniquePipelineCache m_pipelineCache;
    vk::UniqueDescriptorPool m_descriptorPool;
    VulkanGraphicContext m_context;
};

Gui::Gui(const VulkanDevice& device,
         const VulkanGlfwWindow& window,
         const VulkanGraphicContext& context,
         const vk::Instance instance)
    : m_context{ context } {
    core::ThymeLogger().getLogger()->debug("Create Gui Class");
    m_pipelineCache = device.logicalDevice.createPipelineCacheUnique(vk::PipelineCacheCreateInfo());
    m_descriptorPool = createDescriptorPool(device.logicalDevice,
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

    ImGui_ImplGlfw_InitForVulkan(window.getHandler().get(), true);
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = instance;
    initInfo.PhysicalDevice = device.physicalDevice;
    initInfo.Device = device.logicalDevice;
    initInfo.QueueFamily = device.queueFamilyIndices.graphicFamily.value();
    initInfo.Queue = device.getGraphicQueue();

    initInfo.PipelineCache = m_pipelineCache.get();
    initInfo.DescriptorPool = m_descriptorPool.get();
    initInfo.Subpass = VK_SUBPASS_EXTERNAL;
    initInfo.MinImageCount = m_context.imageCount;
    initInfo.ImageCount = m_context.imageCount;
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfo{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &m_context.surfaceFormat.format,
    };
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = [](const auto vkResult) {
        if (vkResult == VK_SUCCESS) {
            return;
        }
        core::ThymeLogger().getLogger()->error("CheckVkResultFn: {}", static_cast<int>(vkResult));
    };
    ImGui_ImplVulkan_Init(&initInfo);
}

void Gui::draw(const vk::CommandBuffer commandBuffer) {

    bool showDemoWindow = true;
    ImGui::ShowDemoWindow(&showDemoWindow);
    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    // Update and Render additional Platform Windows
    if (const auto io = ImGui::GetIO(); io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void Gui::start() const {
    ImGui_ImplVulkan_NewFrame();

    ImGui_ImplGlfw_NewFrame();

    ImGui::NewFrame();
}

Gui::~Gui() noexcept {
    core::ThymeLogger().getLogger()->debug("Destroy Gui Class");

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}


}// namespace th
