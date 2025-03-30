#pragma once

#include <thyme/export_macros.hpp>
#include <thyme/scene/model.hpp>
#include <thyme/platform/vulkan/vulkan_layer.hpp>

#include <string>

namespace th {

class THYME_API Application {
public:
    Application();
    std::string name{ "Thyme" };
    void run();

    vulkan::VulkanLayerStack layers;
    scene::ModelStorage modelStorage;
};

}// namespace th