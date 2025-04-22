#pragma once

#include <vulkan/vulkan.hpp>

#include <thyme/core/layer.hpp>
#include <thyme/core/layer_stack.hpp>

namespace th::vulkan {

using VulkanLayer = Layer<vk::CommandBuffer>;
using VulkanOverlayLayer = OverlayLayer<vk::CommandBuffer>;
using VulkanNonOverlayLayer = NonOverlayLayer<vk::CommandBuffer>;

using VulkanLayerStack = LayerStack<vk::CommandBuffer>;

}// namespace th::vulkan
