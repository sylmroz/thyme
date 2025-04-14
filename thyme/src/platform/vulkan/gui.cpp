#include <thyme/platform/vulkan/gui.hpp>

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace th::vulkan {

Gui::Gui(const Device& device, const VulkanGlfwWindow& window, const vk::Instance& instance) noexcept {
    ImGui_ImplGlfw_InitForVulkan(window.getHandler().get(), true);
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = instance;
    initInfo.PhysicalDevice = device.physicalDevice;
    initInfo.Device = device.logicalDevice.get();
    initInfo.QueueFamily = device.queueFamilyIndices.graphicFamily.value();
    initInfo.Queue = device.logicalDevice->getQueue(device.queueFamilyIndices.graphicFamily.value(), 0);
    m_pipelineCache = device.logicalDevice->createPipelineCacheUnique(vk::PipelineCacheCreateInfo());
    initInfo.PipelineCache = m_pipelineCache.get();
    m_descriptorPool = createDescriptorPool(device.logicalDevice.get(),
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
    initInfo.DescriptorPool = m_descriptorPool.get();
    initInfo.Subpass = 0;
    initInfo.MinImageCount = 2;// VulkanRenderer::maxFramesInFlight;
    initInfo.ImageCount = 2;// VulkanRenderer::maxFramesInFlight;
    initInfo.MSAASamples = static_cast<VkSampleCountFlagBits>(device.maxMsaaSamples);
    initInfo.UseDynamicRendering = true;
    const auto format = device.swapChainSupportDetails.getBestSurfaceFormat().format;
    initInfo.PipelineRenderingCreateInfo =
            vk::PipelineRenderingCreateInfo(0, { format }, findDepthFormat(device.physicalDevice));
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = [](const auto vkResult) {
        if (vkResult == VK_SUCCESS) {
            return;
        }
        TH_API_LOG_INFO("CheckVkResultFn: {}", static_cast<int>(vkResult));
    };
    ImGui_ImplVulkan_Init(&initInfo);
}
void Gui::draw(const vk::CommandBuffer commandBuffer) const noexcept {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    bool showDemoWindow = true;
    ImGui::NewFrame();
    ImGui::ShowDemoWindow(&showDemoWindow);
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    // Update and Render additional Platform Windows
    if (const auto io = ImGui::GetIO(); io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

}// namespace th::vulkan