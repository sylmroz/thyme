#pragma once

#include <thyme/platform/glfw_window.hpp>
#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/platform/vulkan/vulkan_layer.hpp>

#include <vulkan/vulkan.hpp>

namespace th::vulkan {
class Gui : public VulkanNonOverlayLayer {
public:
    explicit Gui(const Device& device, const VulkanGlfwWindow& window, const vk::Instance& instance);

    Gui(Gui&& other) noexcept = delete;
    Gui& operator=(Gui&& other) noexcept = delete;
    Gui(const Gui& other) = delete;
    Gui& operator=(const Gui& other) = delete;

    virtual void draw(vk::CommandBuffer commandBuffer) override;

    virtual void start() override;
    virtual void submit() override {}

    virtual void onEvent(const Event& event) override {};
    virtual void onAttach() override {};
    virtual void onDetach() override {};

    ~Gui() noexcept override;

private:
    vk::UniquePipelineCache m_pipelineCache;
    vk::UniqueDescriptorPool m_descriptorPool;
    vk::UniqueCommandBuffer m_commandBuffer;
};
}// namespace th::vulkan
