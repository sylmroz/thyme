#include <thyme/platform/vulkan/gui.hpp>

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace th::vulkan {

Gui::Gui(const VulkanDevice& device,
         const VulkanGlfwWindow& window,
         VulkanGraphicContext context,
         const vk::Instance instance)
    : VulkanNonOverlayLayer("GUI"), m_context{ context } {
    TH_API_LOG_INFO("Create Gui Class");
    ImGui_ImplGlfw_InitForVulkan(window.getHandler().get(), true);
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = instance;
    initInfo.PhysicalDevice = device.physicalDevice;
    initInfo.Device = device.logicalDevice;
    initInfo.QueueFamily = device.queueFamilyIndices.graphicFamily.value();
    initInfo.Queue = device.getGraphicQueue();
    m_pipelineCache = device.logicalDevice.createPipelineCacheUnique(vk::PipelineCacheCreateInfo());
    initInfo.PipelineCache = m_pipelineCache.get();
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
    initInfo.DescriptorPool = m_descriptorPool.get();
    initInfo.Subpass = VK_SUBPASS_EXTERNAL;
    initInfo.MinImageCount = m_context.imageCount;
    initInfo.ImageCount = m_context.imageCount;
    initInfo.MSAASamples = static_cast<VkSampleCountFlagBits>(device.maxMsaaSamples);
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfo{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &m_context.colorFormat,
    };
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = [](const auto vkResult) {
        if (vkResult == VK_SUCCESS) {
            return;
        }
        TH_API_LOG_ERROR("CheckVkResultFn: {}", static_cast<int>(vkResult));
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

void Gui::start() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

Gui::~Gui() noexcept {
    TH_API_LOG_INFO("Destroy Gui  ");
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}

}// namespace th::vulkan