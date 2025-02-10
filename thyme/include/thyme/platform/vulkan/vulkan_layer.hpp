#pragma once

#include <vulkan/vulkan.hpp>

#include <thyme/core/layer.hpp>
#include <thyme/core/layer_stack.hpp>

namespace Thyme::Vulkan {

using VulkanLayer = Layer<vk::UniqueCommandBuffer>;
using VulkanOverlayLayer = OverlayLayer<vk::UniqueCommandBuffer>;
using VulkanNonOverlayLayer = NonOverlayLayer<vk::UniqueCommandBuffer>;

using VulkanLayerStack = LayerStack<vk::UniqueCommandBuffer>;

}// namespace Thyme::Vulkan
