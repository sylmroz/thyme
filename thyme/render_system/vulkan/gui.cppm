export module th.render_system.vulkan:gui;

import imgui;
//import vulkan;

import th.platform.glfw.glfw_window;
import th.core.logger;
import th.platform.imgui_context;

import :device;
import :graphic_context;
import :utils;

namespace th {
export class Gui final {
public:
    explicit Gui(const VulkanDeviceRAII& device, const GlfwWindow& window, const VulkanGraphicContext& context,
                 vk::Instance instance, Logger& logger);

    Gui(Gui&& other) noexcept = delete;
    auto operator=(Gui&& other) noexcept -> Gui& = delete;
    Gui(const Gui& other) = delete;
    auto operator=(const Gui& other) -> Gui& = delete;

    void draw(vk::CommandBuffer command_buffer);

    void start() const;

    ~Gui() noexcept;

private:
    vk::raii::PipelineCache m_pipelineCache;
    vk::raii::DescriptorPool m_descriptorPool;
    VulkanGraphicContext m_context;
    Logger& m_logger;
};

constexpr auto g_descriptorSets = { vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1000),
                                    vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000),
                                    vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 1000),
                                    vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1000),
                                    vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, 1000),
                                    vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 1000),
                                    vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000),
                                    vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000),
                                    vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1000),
                                    vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, 1000),
                                    vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 1000) };

Gui::Gui(const VulkanDeviceRAII& device, const GlfwWindow& window, const VulkanGraphicContext& context,
         const vk::Instance instance, Logger& logger)
    : m_pipelineCache{ device.logical_device.createPipelineCache(vk::PipelineCacheCreateInfo{}) },
      m_descriptorPool{ createDescriptorPool(device.logical_device, g_descriptorSets) }, m_context{ context },
      m_logger{ logger } {
    logger.debug("Create Gui Class");
    [[maybe_unused]] static ImGuiContext im_gui_context;
    ImGui_ImplGlfw_InitForVulkan(window.getHandler().get(), true);
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = instance;
    init_info.PhysicalDevice = *device.physical_device;
    init_info.Device = *device.logical_device;
    init_info.QueueFamily = device.queue_family_indices.graphic_family.value();
    init_info.Queue = device.getGraphicQueue();

    init_info.PipelineCache = *m_pipelineCache;
    init_info.DescriptorPool = *m_descriptorPool;
    init_info.Subpass = vk::SubpassExternal;
    init_info.MinImageCount = m_context.image_count;
    init_info.ImageCount = m_context.image_count;
    init_info.UseDynamicRendering = true;
    init_info.PipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfo{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &m_context.surface_format.format,
    };
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = [](const auto vk_result) -> auto {
        if (static_cast<vk::Result>(vk_result) == vk::Result::eSuccess) {
            return;
        }
        // m_logger.error("CheckVkResultFn: {}", static_cast<int>(vkResult));
    };
    ImGui_ImplVulkan_Init(&init_info);
}

void Gui::draw(const vk::CommandBuffer command_buffer) {

    bool show_demo_window{ true };
    ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
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
    m_logger.debug("Destroy Gui Class");

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}


}// namespace th
