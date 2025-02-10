#pragma once

#include <thyme/export_macros.hpp>

#include <string>

#include <thyme/platform/vulkan/vulkan_layer.hpp>

namespace Thyme {

class THYME_API Application {
public:
    Application();
    std::string name{ "Thyme" };
    void run();

    Vulkan::VulkanLayerStack layers;
};

}// namespace Thyme