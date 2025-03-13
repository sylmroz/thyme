#pragma once

#include <thyme/export_macros.hpp>

#include <string>

#include <thyme/platform/vulkan/vulkan_layer.hpp>

namespace th {

class THYME_API Application {
public:
    Application();
    std::string name{ "Thyme" };
    void run();

    vulkan::VulkanLayerStack layers;
};

}// namespace Thyme