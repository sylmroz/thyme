#include <thyme/platform/vulkan/utils.hpp>

#include "thyme/core/logger.hpp"
#include "thyme/version.hpp"

#include <map>
#include <set>
#include <vulkan/vulkan.hpp>


using namespace th;
using namespace th::vulkan;

#if !defined(NDEBUG)
PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                                              const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                              const VkAllocationCallbacks* pAllocator,
                                                              VkDebugUtilsMessengerEXT* pMessenger) {
    return pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
                                                           VkDebugUtilsMessengerEXT messenger,
                                                           VkAllocationCallbacks const* pAllocator) {
    return pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(const vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    const vk::DebugUtilsMessageTypeFlagBitsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void*) {
    static std::map<vk::DebugUtilsMessageTypeFlagBitsEXT, std::string_view> messageTypeStringMap = {
        { vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral, "General" },
        { vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance, "Performance" },
        { vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation, "Validation" },
        { vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding, "Device Address Biding" },
    };
    const auto messageTypeStr = messageTypeStringMap[messageType];
    const auto message = fmt::format(
            "[{}]: Name: {}, Message: {}", messageTypeStr, pCallbackData->pMessageIdName, pCallbackData->pMessage);
    switch (messageSeverity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose: TH_API_LOG_TRACE(message); break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo: TH_API_LOG_INFO(message); break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning: TH_API_LOG_WARN(message); break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError: TH_API_LOG_ERROR(message); break;
        default: break;
    }
    return VK_FALSE;
}

vk::DebugUtilsMessengerCreateInfoEXT createDebugUtilsMessengerCreateInfo() {
    const auto flags =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
    const auto typeFlags = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                           | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
                           | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    return vk::DebugUtilsMessengerCreateInfoEXT(vk::DebugUtilsMessengerCreateFlagsEXT(),
                                                flags,
                                                typeFlags,
                                                reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(debugCallback));
}

#endif

static constexpr auto deviceExtensions = std::array{ vk::KHRSwapchainExtensionName };

bool deviceHasAllRequiredExtensions(const vk::PhysicalDevice& physicalDevice) {
    const auto& availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    return std::ranges::all_of(deviceExtensions, [&availableDeviceExtensions](const auto& extension) {
        return std::ranges::any_of(availableDeviceExtensions, [&extension](const auto& instanceExtension) {
            return std::string_view(extension) == std::string_view(instanceExtension.extensionName);
        });
    });
}

UniqueInstance::UniqueInstance(const UniqueInstanceConfig& config) {
    constexpr auto appVersion = vk::makeApiVersion(0, Version::major, Version::minor, Version::patch);
    const vk::ApplicationInfo applicationInfo(
            config.appName.data(), appVersion, config.engineName.data(), appVersion, vk::HeaderVersionComplete);

    auto enabledExtensions = config.instanceExtension;
    enabledExtensions.emplace_back(vk::KHRPortabilityEnumerationExtensionName);
#if !defined(NDEBUG)
    enabledExtensions.emplace_back(vk::EXTDebugUtilsExtensionName);
    constexpr auto validationLayers = std::array{ "VK_LAYER_KHRONOS_validation" };

    const vk::StructureChain instanceCreateInfo(
            vk::InstanceCreateInfo{ vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
                                    &applicationInfo,
                                    validationLayers,
                                    enabledExtensions },
            createDebugUtilsMessengerCreateInfo());
#else
    const vk::StructureChain instanceCreateInfo(vk::InstanceCreateInfo{
            vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR, &applicationInfo, nullptr, enabledExtensions });
#endif

    try {
        instance = vk::createInstanceUnique(instanceCreateInfo.get<vk::InstanceCreateInfo>());
#if !defined(NDEBUG)
        setupDebugMessenger(enabledExtensions);
#endif
    } catch (const vk::SystemError& err) {
        const auto message =
                fmt::format("Failed to create vulkan instance. Message: {}, Code: {}", err.what(), err.code().value());
        TH_API_LOG_ERROR(message);
        throw std::runtime_error(message);
    }
}

#if !defined(NDEBUG)
void UniqueInstance::validateExtensions(const std::vector<const char*>& extensions) {
    const auto extensionProperties = vk::enumerateInstanceExtensionProperties();
    const auto allExtensionSupported = std::ranges::all_of(extensions, [&](const auto& extension) {
        return std::ranges::any_of(extensionProperties, [&](const auto& instanceExtension) {
            return std::string_view(extension) == std::string_view(instanceExtension.extensionName);
        });
    });
    if (!allExtensionSupported) {
        constexpr auto message = "All required extensions are not supported by device";
        TH_API_LOG_ERROR(message);
        throw std::runtime_error(message);
    }
}

void UniqueInstance::setupDebugMessenger(const std::vector<const char*>& extensions) {
    validateExtensions(extensions);

    pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            instance->getProcAddr("vkCreateDebugUtilsMessengerEXT"));
    if (!pfnVkCreateDebugUtilsMessengerEXT) {
        constexpr auto message = "Failed to get vkCreateDebugUtilsMessengerEXT function.";
        TH_API_LOG_ERROR(message);
        throw std::runtime_error(message);
    }

    pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            instance->getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
    if (!pfnVkDestroyDebugUtilsMessengerEXT) {
        constexpr auto message = "Failed to get vkDestroyDebugUtilsMessengerEXT function.";
        TH_API_LOG_ERROR(message);
        throw std::runtime_error(message);
    }

    debugMessenger = instance->createDebugUtilsMessengerEXTUnique(createDebugUtilsMessengerCreateInfo());
}
#endif

QueueFamilyIndices::QueueFamilyIndices(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface) noexcept {
    const auto& queueFamilies = device.getQueueFamilyProperties2();
    for (uint32_t i{ 0 }; i < queueFamilies.size(); i++) {
        const auto& queueFamily = queueFamilies[i];
        const auto& queueFamilyProperties = queueFamily.queueFamilyProperties;
        if (queueFamilyProperties.queueCount <= 0) {
            continue;
        }
        if (queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicFamily = i;
        }
        if (device.getSurfaceSupportKHR(i, surface) != 0u) {
            presentFamily = i;
        }
        if (isCompleted()) {
            break;
        }
    }
}

SwapChainSupportDetails::SwapChainSupportDetails(const vk::PhysicalDevice& device,
                                                 const vk::UniqueSurfaceKHR& surface) {
    capabilities = device.getSurfaceCapabilitiesKHR(*surface);
    formats = device.getSurfaceFormatsKHR(*surface);
    presentModes = device.getSurfacePresentModesKHR(*surface);
}

auto getMaxUsableSampleCount(const vk::PhysicalDevice& device) -> vk::SampleCountFlagBits {
    const auto counts = device.getProperties().limits.framebufferColorSampleCounts
                        & device.getProperties().limits.framebufferDepthSampleCounts;
    if (counts & vk::SampleCountFlagBits::e64) {
        return vk::SampleCountFlagBits::e64;
    }
    if (counts & vk::SampleCountFlagBits::e32) {
        return vk::SampleCountFlagBits::e32;
    }
    if (counts & vk::SampleCountFlagBits::e16) {
        return vk::SampleCountFlagBits::e16;
    }
    if (counts & vk::SampleCountFlagBits::e8) {
        return vk::SampleCountFlagBits::e8;
    }
    if (counts & vk::SampleCountFlagBits::e4) {
        return vk::SampleCountFlagBits::e4;
    }
    if (counts & vk::SampleCountFlagBits::e2) {
        return vk::SampleCountFlagBits::e2;
    }
    return vk::SampleCountFlagBits::e1;
}

std::vector<PhysicalDevice> vulkan::getPhysicalDevices(const vk::UniqueInstance& instance,
                                                       const vk::UniqueSurfaceKHR& surface) {
    static std::map<vk::PhysicalDeviceType, uint32_t> priorities = {
        { vk::PhysicalDeviceType::eOther, 0 },       { vk::PhysicalDeviceType::eCpu, 1 },
        { vk::PhysicalDeviceType::eVirtualGpu, 2 },  { vk::PhysicalDeviceType::eIntegratedGpu, 3 },
        { vk::PhysicalDeviceType::eDiscreteGpu, 4 },
    };

    std::vector<PhysicalDevice> physicalDevices;
    for (const auto& device : instance->enumeratePhysicalDevices()) {
        const auto queueFamilyIndex = QueueFamilyIndices(device, *surface);
        const auto deviceSupportExtensions = deviceHasAllRequiredExtensions(device);
        const auto swapChainSupportDetails = SwapChainSupportDetails(device, surface);
        const auto maxMsaaSamples = getMaxUsableSampleCount(device);
        if (queueFamilyIndex.isCompleted() && deviceSupportExtensions && swapChainSupportDetails.isValid()
            && device.getFeatures().samplerAnisotropy) {
            physicalDevices.emplace_back(device, queueFamilyIndex, swapChainSupportDetails, maxMsaaSamples);
        }
    }

    if (physicalDevices.empty()) {
        constexpr auto message = "No physical device exist which met all requirements!";
        TH_API_LOG_ERROR(message);
        throw std::runtime_error(message);
    }

    std::ranges::sort(physicalDevices, [](const auto& device1, const auto& device2) {
        const auto dt1 = device1.physicalDevice.getProperties().deviceType;
        const auto dt2 = device2.physicalDevice.getProperties().deviceType;
        return priorities[dt1] > priorities[dt2];
    });

    return physicalDevices;
}

[[nodiscard]] vk::UniqueDevice th::vulkan::PhysicalDevice::createLogicalDevice() const {
    const std::set indices = { queueFamilyIndices.graphicFamily.value(), queueFamilyIndices.presentFamily.value() };
    std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos;
    for (const auto ind : indices) {
        float queuePriority{ 1.0 };
        deviceQueueCreateInfos.emplace_back(vk::DeviceQueueCreateFlags(), ind, 1, &queuePriority);
    }

    const auto features = physicalDevice.getFeatures();
    return physicalDevice.createDeviceUnique(vk::DeviceCreateInfo(
            vk::DeviceCreateFlags(), deviceQueueCreateInfos, nullptr, deviceExtensions, &features));
}

SwapChainData::SwapChainData(const Device& device,
                             const SwapChainSettings& swapChainSettings,
                             const vk::Extent2D& swapChainExtent,
                             const vk::UniqueRenderPass& renderPass,
                             const vk::UniqueSurfaceKHR& surface,
                             const vk::UniqueImageView& colorImageView,
                             const vk::UniqueImageView& depthImageView,
                             const vk::SwapchainKHR& oldSwapChain) {
    const auto& [surfaceFormat, presetMode, imageCount] = swapChainSettings;
    [[maybe_unused]] const auto& [physicalDevice,
                                  logicalDevice,
                                  queueFamilyIndices,
                                  swapChainSupportDetails,
                                  maxMsaaSamples] = device;
    const auto swapChainCreateInfo = [&] {
        auto info = vk::SwapchainCreateInfoKHR(vk::SwapchainCreateFlagsKHR(),
                                               *surface,
                                               imageCount,
                                               surfaceFormat.format,
                                               surfaceFormat.colorSpace,
                                               swapChainExtent,
                                               1,
                                               vk::ImageUsageFlagBits::eColorAttachment);
        info.preTransform = swapChainSupportDetails.capabilities.currentTransform;
        info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        info.presentMode = presetMode;
        info.clipped = vk::True;
        const auto indices = { queueFamilyIndices.graphicFamily.value(), queueFamilyIndices.presentFamily.value() };
        if (queueFamilyIndices.graphicFamily.value() != queueFamilyIndices.presentFamily.value()) {
            info.imageSharingMode = vk::SharingMode::eConcurrent;
            info.setQueueFamilyIndices(indices);
        } else {
            info.imageSharingMode = vk::SharingMode::eExclusive;
        }
        info.oldSwapchain = oldSwapChain;
        return info;
    }();

    swapChain = logicalDevice->createSwapchainKHRUnique(swapChainCreateInfo);
    swapChainFrame = logicalDevice->getSwapchainImagesKHR(*swapChain)
                     | std::views::transform([&](vk::Image& image) -> SwapChainFrame {
                           auto imageView = createImageView(
                                   *logicalDevice, image, surfaceFormat.format, vk::ImageAspectFlagBits::eColor);
                           const auto attachments = std::array{ *colorImageView, *depthImageView, *imageView };
                           auto frameBuffer = logicalDevice->createFramebufferUnique(
                                   vk::FramebufferCreateInfo(vk::FramebufferCreateFlagBits(),
                                                             *renderPass,
                                                             attachments,
                                                             swapChainExtent.width,
                                                             swapChainExtent.height,
                                                             1));
                           return SwapChainFrame{ std::move(image), std::move(imageView), std::move(frameBuffer) };
                       })
                     | std::ranges::to<decltype(swapChainFrame)>();
}


auto vulkan::createRenderPass(const vk::UniqueDevice& logicalDevice, const vk::Format colorFormat,
                              const vk::Format depthFormat, const vk::SampleCountFlagBits samples)
        -> vk::UniqueRenderPass {

    constexpr auto colorAttachmentRef = vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);
    constexpr auto depthAttachmentRef = vk::AttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    constexpr auto colorAttachmentResolveRef = vk::AttachmentReference(2, vk::ImageLayout::eColorAttachmentOptimal);
    const auto subpassDescription = vk::SubpassDescription(vk::SubpassDescriptionFlagBits(),
                                                           vk::PipelineBindPoint::eGraphics,
                                                           {},
                                                           { colorAttachmentRef },
                                                           { colorAttachmentResolveRef },
                                                           &depthAttachmentRef);

    constexpr auto subpassDependency = vk::SubpassDependency(
            vk::SubpassExternal,
            0,
            vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
            vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
            vk::AccessFlagBits::eNone,
            vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

    const auto colorAttachment = vk::AttachmentDescription(vk::AttachmentDescriptionFlagBits(),
                                                           colorFormat,
                                                           samples,
                                                           vk::AttachmentLoadOp::eClear,
                                                           vk::AttachmentStoreOp::eStore,
                                                           vk::AttachmentLoadOp::eDontCare,
                                                           vk::AttachmentStoreOp::eDontCare,
                                                           vk::ImageLayout::eUndefined,
                                                           vk::ImageLayout::eColorAttachmentOptimal);

    const auto depthAttachment = vk::AttachmentDescription(vk::AttachmentDescriptionFlagBits(),
                                                           depthFormat,
                                                           samples,
                                                           vk::AttachmentLoadOp::eClear,
                                                           vk::AttachmentStoreOp::eDontCare,
                                                           vk::AttachmentLoadOp::eDontCare,
                                                           vk::AttachmentStoreOp::eDontCare,
                                                           vk::ImageLayout::eUndefined,
                                                           vk::ImageLayout::eDepthStencilAttachmentOptimal);
    const auto colorAttachmentResolve = vk::AttachmentDescription(vk::AttachmentDescriptionFlagBits(),
                                                                  colorFormat,
                                                                  vk::SampleCountFlagBits::e1,
                                                                  vk::AttachmentLoadOp::eDontCare,
                                                                  vk::AttachmentStoreOp::eStore,
                                                                  vk::AttachmentLoadOp::eDontCare,
                                                                  vk::AttachmentStoreOp::eDontCare,
                                                                  vk::ImageLayout::eUndefined,
                                                                  vk::ImageLayout::ePresentSrcKHR);

    const auto attachments = std::array{ colorAttachment, depthAttachment, colorAttachmentResolve };
    return logicalDevice->createRenderPassUnique(vk::RenderPassCreateInfo(
            vk::RenderPassCreateFlagBits(), attachments, { subpassDescription }, { subpassDependency }));
}

auto vulkan::createGraphicsPipeline(const GraphicPipelineCreateInfo& graphicPipelineCreateInfo) -> vk::UniquePipeline {
    const auto& [logicalDevice, renderPass, pipelineLayout, samples, shaderStages] = graphicPipelineCreateInfo;
    constexpr auto dynamicStates = std::array{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    const auto dynamicStateCreateInfo =
            vk::PipelineDynamicStateCreateInfo(vk::PipelineDynamicStateCreateFlagBits(), dynamicStates);
    constexpr auto bindingDescription = Vertex::getBindingDescription();
    constexpr auto attributeDescriptions = Vertex::getAttributeDescriptions();
    const auto vertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo(
            vk::PipelineVertexInputStateCreateFlagBits(), { bindingDescription }, attributeDescriptions);
    constexpr auto inputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo(
            vk::PipelineInputAssemblyStateCreateFlagBits(), vk::PrimitiveTopology::eTriangleList, vk::False);
    constexpr auto viewportState =
            vk::PipelineViewportStateCreateInfo(vk::PipelineViewportStateCreateFlagBits(), 1, nullptr, 1, nullptr);
    constexpr auto rasterizer = vk::PipelineRasterizationStateCreateInfo(vk::PipelineRasterizationStateCreateFlagBits(),
                                                                         vk::False,
                                                                         vk::False,
                                                                         vk::PolygonMode::eFill,
                                                                         vk::CullModeFlagBits::eBack,
                                                                         vk::FrontFace::eCounterClockwise,
                                                                         vk::False,
                                                                         0.0f,
                                                                         0.0f,
                                                                         0.0f,
                                                                         1.0f);
    const auto multisampling = vk::PipelineMultisampleStateCreateInfo(
            vk::PipelineMultisampleStateCreateFlagBits(), samples, vk::False, 1.0f, nullptr, vk::False, vk::False);
    constexpr auto colorBlendAttachments = vk::PipelineColorBlendAttachmentState(
            vk::False,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                    | vk::ColorComponentFlagBits::eB);
    const auto colorBlendStateCreateInfo =
            vk::PipelineColorBlendStateCreateInfo(vk::PipelineColorBlendStateCreateFlagBits(),
                                                  vk::False,
                                                  vk::LogicOp::eClear,
                                                  { colorBlendAttachments },
                                                  std::array{ 0.0f, 0.0f, 0.0f, 0.0f });
    const auto deptStencilStateCreateInfo =
            vk::PipelineDepthStencilStateCreateInfo(vk::PipelineDepthStencilStateCreateFlagBits(),
                                                    vk::True,
                                                    vk::True,
                                                    vk::CompareOp::eLess,
                                                    vk::False,
                                                    vk::False,
                                                    {},
                                                    {},
                                                    0.0f,
                                                    1.0f);
    return logicalDevice
            ->createGraphicsPipelineUnique({},
                                           vk::GraphicsPipelineCreateInfo(vk::PipelineCreateFlagBits(),
                                                                          shaderStages,
                                                                          &vertexInputStateCreateInfo,
                                                                          &inputAssemblyStateCreateInfo,
                                                                          nullptr,
                                                                          &viewportState,
                                                                          &rasterizer,
                                                                          &multisampling,
                                                                          &deptStencilStateCreateInfo,
                                                                          &colorBlendStateCreateInfo,
                                                                          &dynamicStateCreateInfo,
                                                                          *pipelineLayout,
                                                                          *renderPass,
                                                                          0))
            .value;
}
