module;

#include <vector>

#include <fmt/format.h>
#include <vulkan/vulkan.hpp>

export module thyme.platform.vulkan_renderer;

export namespace Thyme::Vulkan {

struct UniqueInstanceConfig {
    std::string_view engineName;
    std::string_view appName;
    std::vector<const char*> instanceLayers;
    std::vector<const char*> instanceExtension;
};

class UniqueInstance {
public:
    explicit UniqueInstance(const UniqueInstanceConfig& config);
    static void validateExtensions();
    vk::UniqueInstance instance;

private:
#if !defined(NDEBUG)
    void setupDebugMessenger();
    vk::UniqueDebugUtilsMessengerEXT debugMessenger;
#endif
};

// TODO - the class should support more queue family flags like eSparseBinding
struct QueueFamilyIndices {
    explicit QueueFamilyIndices(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface);

    std::optional<uint32_t> graphicFamily;
    std::optional<uint32_t> presentFamily;

    [[nodiscard]] constexpr bool isCompleted() const noexcept {
        return graphicFamily.has_value() && presentFamily.has_value();
    }
};

class PhysicalDevice {
public:
    explicit PhysicalDevice(const vk::PhysicalDevice& physicalDevice,
                            const QueueFamilyIndices& queueFamilyIndices) noexcept
        : physicalDevice{ physicalDevice }, queueFamilyIndices{ queueFamilyIndices } {}

    vk::PhysicalDevice physicalDevice;
    QueueFamilyIndices queueFamilyIndices;
    vk::Device logicalDevice;

    [[nodiscard]] vk::UniqueDevice createLogicalDevice() const;
};

std::vector<PhysicalDevice> getPhysicalDevices(const vk::UniqueInstance& instance, const vk::UniqueSurfaceKHR& surface);

// TODO - read from config, last selected device
class PhysicalDevicesManager {
public:
    explicit PhysicalDevicesManager(const std::vector<PhysicalDevice>& devices) : m_physicalDevices(devices) {
        m_selectedDevice = m_physicalDevices.begin();
    }

    [[nodiscard]] auto& getSelectedDevice() const noexcept {
        return *m_selectedDevice;
    }

    [[nodiscard]] auto& getSelectedDevice() noexcept {
        return *m_selectedDevice;
    }

    [[nodiscard]] auto& getDevicesList() const noexcept {
        return m_physicalDevices;
    }

    [[nodiscard]] auto& getDevicesList() noexcept {
        return m_physicalDevices;
    }

    void selectDevice(uint32_t index) {
        if (index - 1 >= m_physicalDevices.size()) {
            const auto message =
                    fmt::format("Selecting physical device failed! Selected index is {}, but devices are {}",
                                index,
                                m_physicalDevices.size());
            throw std::runtime_error(message);
        }
        m_selectedDevice = m_physicalDevices.begin() + index - 1;
    }

private:
    std::vector<PhysicalDevice> m_physicalDevices;
    std::vector<PhysicalDevice>::iterator m_selectedDevice;
};

}// namespace Thyme::Vulkan