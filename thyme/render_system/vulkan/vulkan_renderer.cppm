export module th.render_system.vulkan:renderer;

import std;

import vulkan;

import th.scene.camera;
import th.scene.model;
import th.core.logger;

import :buffer;
import :command_buffers;
import :device;
import :graphic_pipeline;
import :graphic_context;
import :swapchain;
import :model;
import :texture;
import :uniform_buffer_object;
import :utils;
import :gui;

namespace th {

export class VulkanRenderer {
public:
    explicit VulkanRenderer(const VulkanDeviceRAII& device, VulkanSwapchain& swapchain, ModelStorage& model_storage,
                            Camera& camera, Gui& gui, const VulkanGraphicContext& context,
                            VulkanCommandBuffersPool& command_buffers_pool, Logger& logger) noexcept;

    void draw(const VulkanDeviceRAII& device);

private:
    Gui& m_gui;
    VulkanSwapchain& m_swapchain;
    VulkanCommandBuffersPool& m_command_buffers_pool;
    std::vector<std::unique_ptr<VulkanGraphicPipeline>> m_pipelines;
    vk::raii::DescriptorPool m_descriptor_pool;
    std::unique_ptr<GradientPipeline> m_gradient_pipeline;

    VulkanDepthImageMemory m_depth_image_memory;
    VulkanColorImageMemory m_color_image_memory;
    VulkanColorImageMemory m_resolve_color_image_memory;

    std::vector<VulkanModel> m_models;
    std::reference_wrapper<Camera> m_camera;

    std::reference_wrapper<ModelStorage> m_model_storage;
    VulkanUniformBuffer<CameraMatrices> m_camera_matrices;
};

}// namespace th
