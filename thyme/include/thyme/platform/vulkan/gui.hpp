#pragma once

#include <thyme/platform/glfw_window.hpp>
#include <thyme/platform/vulkan/vulkan_device.hpp>"
#include <thyme/platform/vulkan/vulkan_layer.hpp>
#include <thyme/platform/vulkan/vulkan_graphic_context.hpp>

#include <vulkan/vulkan.hpp>

namespace th::vulkan {
class Gui final: public VulkanNonOverlayLayer {
public:
    explicit Gui(const VulkanDevice& device, const VulkanGlfwWindow& window, VulkanGraphicContext context, vk::Instance instance);

    Gui(Gui&& other) noexcept = delete;
    Gui& operator=(Gui&& other) noexcept = delete;
    Gui(const Gui& other) = delete;
    Gui& operator=(const Gui& other) = delete;

    void draw(vk::CommandBuffer commandBuffer) override;

    void start() override;
    void submit() override {}

    void onEvent(const Event& event) override {};
    void onAttach() override {};
    void onDetach() override {};

    ~Gui() noexcept override;

private:
    vk::UniquePipelineCache m_pipelineCache;
    vk::UniqueDescriptorPool m_descriptorPool;
    vk::UniqueCommandBuffer m_commandBuffer;
    VulkanGraphicContext m_context;
};
}// namespace th::vulkan
