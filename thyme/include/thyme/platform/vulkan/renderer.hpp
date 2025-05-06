#pragma once

#include <thyme/platform/glfw_window.hpp>
#include <thyme/platform/vulkan/graphic_pipeline.hpp>
#include <thyme/platform/vulkan/gui.hpp>
#include <thyme/platform/vulkan/swapchain.hpp>
#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/renderer/renderer.hpp>
#include <thyme/scene/camera.hpp>
#include <thyme/scene/model.hpp>

#include <vulkan/vulkan.hpp>

namespace th::vulkan {

class VulkanRenderer final: public renderer::Renderer {
public:
    explicit VulkanRenderer(const VulkanDevice& device, VulkanSwapChain& swapChain, scene::ModelStorage& modelStorage,
                            scene::Camera& camera, Gui& gui, const VulkanGraphicContext& context) noexcept;

    void draw() override;

private:
    VulkanDevice m_device;
    Gui& m_gui;
    std::vector<vk::UniqueCommandBuffer> m_commandBuffers;
    size_t m_commandBufferIndex{ 0 };

    VulkanSwapChain& m_swapChain;
    VulkanGraphicContext m_context;

    std::vector<std::unique_ptr<GraphicPipeline>> m_pipelines;
};
}// namespace th::vulkan