#include <thyme/platform/vulkan/utils.hpp>

#include "thyme/core/logger.hpp"
#include "thyme/version.hpp"

#include <map>
#include <set>
#include <vulkan/vulkan.hpp>

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

namespace th::vulkan {

static constexpr auto s_deviceExtensions =
        std::array{ vk::KHRSwapchainExtensionName, vk::KHRDynamicRenderingExtensionName };

static bool deviceHasAllRequiredExtensions(const vk::PhysicalDevice& physicalDevice) {
    const auto& availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    return std::ranges::all_of(s_deviceExtensions, [&availableDeviceExtensions](const auto& extension) {
        return std::ranges::any_of(availableDeviceExtensions, [&extension](const auto& instanceExtension) {
            return std::string_view(extension) == std::string_view(instanceExtension.extensionName);
        });
    });
}

UniqueInstance::UniqueInstance(const UniqueInstanceConfig& config) {
    constexpr auto appVersion = vk::makeApiVersion(0, version::major, version::minor, version::patch);
    const vk::ApplicationInfo applicationInfo(
            config.appName.data(), appVersion, config.engineName.data(), appVersion, vk::HeaderVersionComplete);

    auto enabledExtensions = config.instanceExtension;
    enabledExtensions.emplace_back(vk::KHRPortabilityEnumerationExtensionName);
#if !defined(NDEBUG)
    enabledExtensions.emplace_back(vk::EXTDebugUtilsExtensionName);
    constexpr auto validationLayers = std::array{ "VK_LAYER_KHRONOS_validation" };
    std::vector<vk::ValidationFeatureEnableEXT> enabled{ vk::ValidationFeatureEnableEXT::eSynchronizationValidation };
    vk::ValidationFeaturesEXT validationFeatures(enabled);
    const vk::StructureChain instanceCreateInfo(
            vk::InstanceCreateInfo{ vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
                                    &applicationInfo,
                                    validationLayers,
                                    enabledExtensions },
            createDebugUtilsMessengerCreateInfo(),
            validationFeatures);
#else
    const vk::StructureChain instanceCreateInfo(vk::InstanceCreateInfo{
            vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR, &applicationInfo, nullptr, enabledExtensions });
#endif

    try {
        m_instance = vk::createInstanceUnique(instanceCreateInfo.get<vk::InstanceCreateInfo>());
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
            m_instance->getProcAddr("vkCreateDebugUtilsMessengerEXT"));
    if (!pfnVkCreateDebugUtilsMessengerEXT) {
        constexpr auto message = "Failed to get vkCreateDebugUtilsMessengerEXT function.";
        TH_API_LOG_ERROR(message);
        throw std::runtime_error(message);
    }

    pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            m_instance->getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
    if (!pfnVkDestroyDebugUtilsMessengerEXT) {
        constexpr auto message = "Failed to get vkDestroyDebugUtilsMessengerEXT function.";
        TH_API_LOG_ERROR(message);
        throw std::runtime_error(message);
    }

    debugMessenger = m_instance->createDebugUtilsMessengerEXTUnique(createDebugUtilsMessengerCreateInfo());
}
#endif

QueueFamilyIndices::QueueFamilyIndices(const vk::PhysicalDevice device, const vk::SurfaceKHR surface) noexcept {
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

SwapChainSupportDetails::SwapChainSupportDetails(const vk::PhysicalDevice& device, const vk::SurfaceKHR surface) {
    capabilities = device.getSurfaceCapabilitiesKHR(surface);
    formats = device.getSurfaceFormatsKHR(surface);
    presentModes = device.getSurfacePresentModesKHR(surface);
}

static auto getMaxUsableSampleCount(const vk::PhysicalDevice& device) -> vk::SampleCountFlagBits {
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

FrameDataList::FrameDataList(const vk::Device logicalDevice, const uint32_t maxFrames) noexcept {
    for (uint32_t i{ 0 }; i < maxFrames; ++i) {
        m_frameDataList.emplace_back(InternalFrameData{
                .imageAvailableSemaphore = logicalDevice.createSemaphoreUnique(vk::SemaphoreCreateInfo()),
                .renderFinishedSemaphore = logicalDevice.createSemaphoreUnique(vk::SemaphoreCreateInfo()),
                .fence = logicalDevice.createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)),
        });
    }
}

auto createRenderPass(const vk::Device logicalDevice, const vk::Format colorFormat, const vk::Format depthFormat,
                      const vk::SampleCountFlagBits samples) -> vk::UniqueRenderPass {
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
    return logicalDevice.createRenderPassUnique(vk::RenderPassCreateInfo(
            vk::RenderPassCreateFlagBits(), attachments, { subpassDescription }, { subpassDependency }));
}

auto createGraphicsPipeline(const GraphicPipelineCreateInfo& graphicPipelineCreateInfo) -> vk::UniquePipeline {
    const auto& [logicalDevice, renderPass, pipelineLayout, samples, pipelineRenderingCreateInfo, shaderStages] =
            graphicPipelineCreateInfo;
    constexpr auto dynamicStates = std::array{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    const auto dynamicStateCreateInfo =
            vk::PipelineDynamicStateCreateInfo(vk::PipelineDynamicStateCreateFlagBits(), dynamicStates);
    constexpr auto bindingDescription = getBindingDescription();
    constexpr auto attributeDescriptions = getAttributeDescriptions();
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
    constexpr auto deptStencilStateCreateInfo =
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
            .createGraphicsPipelineUnique({},
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
                                                                         pipelineLayout,
                                                                         {},
                                                                         {},
                                                                         {},
                                                                         {},
                                                                         &pipelineRenderingCreateInfo))
            .value;
}

void transitImageLayout(const vk::CommandBuffer commandBuffer, const vk::Image image,
                        const ImageLayoutTransition layoutTransition,
                        const ImagePipelineStageTransition stageTransition,
                        const ImageAccessFlagsTransition accessFlagsTransition, const vk::ImageAspectFlags aspectFlags,
                        const uint32_t mipLevels) {
    const auto [srcAccessFlag, dstAccessFlag] = accessFlagsTransition;
    const auto [oldLayout, newLayout] = layoutTransition;
    const auto [srcStages, dstStages] = stageTransition;
    const auto barrier = vk::ImageMemoryBarrier(srcAccessFlag,
                                                dstAccessFlag,
                                                oldLayout,
                                                newLayout,
                                                vk::QueueFamilyIgnored,
                                                vk::QueueFamilyIgnored,
                                                image,
                                                vk::ImageSubresourceRange(aspectFlags, 0, mipLevels, 0, 1));
    commandBuffer.pipelineBarrier(srcStages, dstStages, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);
}

void transitImageLayout(const vk::CommandBuffer commandBuffer, const vk::Image image, const vk::ImageLayout oldLayout,
                        const vk::ImageLayout newLayout, const uint32_t mipLevels) {
    const auto [barrier, srcPipelineStage, dstPipelineStage] = [&] {
        const auto createBarrier = [&](const vk::AccessFlags srcAccessFlag, const vk::AccessFlags dstAccessFlag) {
            return vk::ImageMemoryBarrier(
                    srcAccessFlag,
                    dstAccessFlag,
                    oldLayout,
                    newLayout,
                    vk::QueueFamilyIgnored,
                    vk::QueueFamilyIgnored,
                    image,
                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 1));
        };

        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
            return std::tuple(createBarrier(vk::AccessFlags(), vk::AccessFlagBits::eTransferWrite),
                              vk::PipelineStageFlagBits::eTopOfPipe,
                              vk::PipelineStageFlagBits::eTransfer);
        }
        if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            return std::tuple(createBarrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead),
                              vk::PipelineStageFlagBits::eTransfer,
                              vk::PipelineStageFlagBits::eFragmentShader);
        }
        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
            return std::tuple(
                    createBarrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eColorAttachmentRead),
                    vk::PipelineStageFlagBits::eTransfer,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput);
        }
        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            return std::tuple(createBarrier(vk::AccessFlags(), vk::AccessFlagBits::eDepthStencilAttachmentWrite),
                              vk::PipelineStageFlagBits::eTopOfPipe,
                              vk::PipelineStageFlagBits::eEarlyFragmentTests);
        }
        if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal && newLayout == vk::ImageLayout::ePresentSrcKHR) {
            return std::tuple(createBarrier(vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits()),
                              vk::PipelineStageFlagBits::eColorAttachmentOutput,
                              vk::PipelineStageFlagBits::eBottomOfPipe);
        }
        throw std::runtime_error("failed to transit image layout!");
    }();
    commandBuffer.pipelineBarrier(
            srcPipelineStage, dstPipelineStage, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);
}

auto findSupportedImageFormat(const vk::PhysicalDevice device, const std::span<const vk::Format> formats,
                              const vk::ImageTiling imageTiling, const vk::FormatFeatureFlags features) -> vk::Format {
    for (const auto format : formats) {
        const auto properties = device.getFormatProperties(format);
        if (imageTiling == vk::ImageTiling::eLinear && (properties.linearTilingFeatures & features) == features) {
            return format;
        }
        if (imageTiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find image format");
}
void setCommandBufferFrameSize(const vk::CommandBuffer commandBuffer, const vk::Extent2D frameSize) {
    commandBuffer.setViewport(0,
                              { vk::Viewport(0.0f,
                                             0.0f,
                                             static_cast<float>(frameSize.width),
                                             static_cast<float>(frameSize.height),
                                             0.0f,
                                             1.0f) });
    commandBuffer.setScissor(0, { vk::Rect2D(vk::Offset2D(0, 0), frameSize) });
}

}// namespace th::vulkan