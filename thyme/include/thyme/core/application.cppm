module;

#include <thyme/export_macros.hpp>

#include <string>

export module thyme.core.application;

import thyme.platform.vulkan_layer;

namespace Thyme {

export class THYME_API Application {
public:
    Application();
    std::string name{ "Thyme" };
    void run();

    Vulkan::VulkanLayerStack layers;
};

}// namespace Thyme