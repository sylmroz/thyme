#pragma once

#include "thyme/core/logger.hpp"
#include "thyme/version.hpp"

#include <string_view>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace Thyme::Vulkan {

struct UniqueInstanceConfig {
    std::string_view engineName;
    std::string_view appName;
    std::vector<const char*> instanceLayers;
    std::vector<const char*> instanceExtension;
};

class UniqueInstance {
public:
    UniqueInstance(const UniqueInstanceConfig& config) {
        constexpr auto appVersion = vk::makeApiVersion(0, Version::major, Version::minor, Version::patch);
        constexpr auto vulkanVersion = vk::makeApiVersion(0, 1, 3, 290);
        vk::ApplicationInfo applicationInfo(
                config.appName.data(), appVersion, config.engineName.data(), appVersion, vulkanVersion);

        vk::InstanceCreateInfo instanceCreateInfo(
                vk::InstanceCreateFlags(), &applicationInfo, config.instanceLayers, config.instanceExtension);
        try {
            instance = vk::createInstanceUnique(instanceCreateInfo);
        } catch (vk::SystemError err) {
            TH_API_LOG_ERROR("Failed to create vulkan instance. Message: {}, Code: {}", err.what(), err.code().value());
            throw std::runtime_error("Failed to create vulkan instance.");
        }
    }
    vk::UniqueInstance instance;
};

}// namespace Thyme::Vulkan
