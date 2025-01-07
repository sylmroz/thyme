module;

#include <vulkan/vulkan.hpp>

export module thyme.platform.vulkan_layer;

import thyme.core.layer;
import thyme.core.layer_stack;

export namespace Thyme::Vulkan {

using VulkanLayer = Layer<vk::UniqueCommandBuffer>;
using VulkanOverlayLayer = OverlayLayer<vk::UniqueCommandBuffer>;
using VulkanNonOverlayLayer = NonOverlayLayer<vk::UniqueCommandBuffer>;

using VulkanLayerStack = LayerStack<vk::UniqueCommandBuffer>;

}// namespace Thyme::Vulkan
