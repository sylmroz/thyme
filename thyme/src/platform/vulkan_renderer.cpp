#include "thyme/platform/vulkan_renderer.hpp"

Thyme::Vulkan::UniqueInstance::UniqueInstance(const UniqueInstanceConfig& config) {
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
