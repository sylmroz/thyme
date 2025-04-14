#pragma once

#include <thyme/platform/glfw_window.hpp>
#include <thyme/platform/vulkan/utils.hpp>

#include <vulkan/vulkan.hpp>

namespace th::vulkan {
class Gui {
public:
    explicit Gui(const Device& device,
                 const VulkanGlfwWindow& window,
                 const vk::Instance& instance) noexcept;
    void draw(const vk::CommandBuffer commandBuffer) const noexcept;

private:
    vk::UniquePipelineCache m_pipelineCache;
    vk::UniqueDescriptorPool m_descriptorPool;
};
}// namespace th::vulkan
