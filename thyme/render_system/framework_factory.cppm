module;

#include "thyme/platform/glfw_window.hpp"

import th.render_system.framework;
import th.render_system.vulkan.framework;

export module th.render_system.framework_factory;

namespace th::render_system {

export template <BackendType type = BackendType::vulkan, bool hasSwapChain = true>
auto createFramework(const Framework::InitInfo& initInfo) -> std::unique_ptr<Framework> {
    if constexpr (type == BackendType::vulkan) {
        if constexpr (hasSwapChain) {
            const auto extensions = VulkanGlfwWindow::getExtensions();
            return std::make_unique<vulkan::Framework>(initInfo, extensions);
        } else {
            return std::make_unique<vulkan::Framework>(initInfo);
        }
    }
    std::unreachable();
}
}
