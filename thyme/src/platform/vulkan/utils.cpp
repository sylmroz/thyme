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

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(const vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                      const vk::DebugUtilsMessageTypeFlagsEXT messageType,
                                                      const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                      void*) {
    const auto messageTypeStr = vk::to_string(messageType);
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


auto createDebugUtilsMessengerCreateInfo() -> vk::DebugUtilsMessengerCreateInfoEXT {
    constexpr auto flags =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
    constexpr auto typeFlags = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                               | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
                               | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    return vk::DebugUtilsMessengerCreateInfoEXT{
        .messageSeverity = flags,
        .messageType = typeFlags,
        .pfnUserCallback = debugCallback,
    };
}

#endif

namespace th::vulkan {

UniqueInstance::UniqueInstance(const UniqueInstanceConfig& config) {
    constexpr auto appVersion = vk::makeApiVersion(0, version::major, version::minor, version::patch);
    const auto applicationInfo = vk::ApplicationInfo{
        .pApplicationName = config.appName.data(),
        .applicationVersion = appVersion,
        .pEngineName = config.engineName.data(),
        .engineVersion = appVersion,
        .apiVersion = vk::HeaderVersionComplete,
    };

    auto enabledExtensions = config.instanceExtension;
    enabledExtensions.emplace_back(vk::KHRPortabilityEnumerationExtensionName);
#if !defined(NDEBUG)
    enabledExtensions.emplace_back(vk::EXTDebugUtilsExtensionName);
    constexpr auto layerName = "VK_LAYER_KHRONOS_validation";
    constexpr auto validationLayers = std::array{ layerName };
    constexpr auto enabled = std::array{ vk::ValidationFeatureEnableEXT::eSynchronizationValidation };
    const auto validationFeatures = vk::ValidationFeaturesEXT{
        .enabledValidationFeatureCount = enabled.size(),
        .pEnabledValidationFeatures = enabled.data(),
    };
    const vk::StructureChain instanceCreateInfo(
            vk::InstanceCreateInfo{
                    .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
                    .pApplicationInfo = &applicationInfo,
                    .enabledLayerCount = validationLayers.size(),
                    .ppEnabledLayerNames = validationLayers.data(),
                    .enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
                    .ppEnabledExtensionNames = enabledExtensions.data(),
            },
            createDebugUtilsMessengerCreateInfo(),
            validationFeatures);
#else
    const vk::StructureChain instanceCreateInfo(vk::InstanceCreateInfo{
            .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
            .pApplicationInfo = &applicationInfo,
            .enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
            .ppEnabledExtensionNames = enabledExtensions.data(),
    });
#endif

    try {
        m_instance = vk::createInstanceUnique(instanceCreateInfo.get<vk::InstanceCreateInfo>());
#if !defined(NDEBUG)
        setupDebugMessenger(enabledExtensions);
#endif
    } catch (const vk::SystemError& err) {
        const auto message =
                fmt::format("Failed to create vulkan instance.\n Message: {}\n Code message: {}\n Code: {}",
                            err.what(),
                            err.code().message(),
                            err.code().value());
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

QueueFamilyIndices::QueueFamilyIndices(const vk::PhysicalDevice device, const vk::SurfaceKHR surface) {
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

auto createGraphicsPipeline(const GraphicPipelineCreateInfo& graphicPipelineCreateInfo) -> vk::UniquePipeline {
    const auto& [logicalDevice, pipelineLayout, samples, pipelineRenderingCreateInfo, shaderStages] =
            graphicPipelineCreateInfo;
    constexpr auto dynamicStates = std::array{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    const auto dynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };
    constexpr auto bindingDescription = getBindingDescription();
    constexpr auto attributeDescriptions = getAttributeDescriptions();
    const auto vertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data(),
    };
    constexpr auto inputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo{
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = vk::False,
    };
    constexpr auto viewportState = vk::PipelineViewportStateCreateInfo{
        .viewportCount = 1,
        .scissorCount = 1,
    };
    constexpr auto rasterizer = vk::PipelineRasterizationStateCreateInfo{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };
    const auto multisampling = vk::PipelineMultisampleStateCreateInfo{
        .rasterizationSamples = samples,
        .sampleShadingEnable = vk::False,
        .minSampleShading = 1.0f,
        .alphaToCoverageEnable = vk::False,
        .alphaToOneEnable = vk::False,
    };
    constexpr auto colorBlendAttachments = vk::PipelineColorBlendAttachmentState{
        .blendEnable = vk::False,
        .srcColorBlendFactor = vk::BlendFactor::eOne,
        .dstColorBlendFactor = vk::BlendFactor::eZero,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR
                          | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
    };
    const auto colorBlendStateCreateInfo =
            vk::PipelineColorBlendStateCreateInfo{ .logicOpEnable = vk::False,
                                                   .logicOp = vk::LogicOp::eCopy,
                                                   .attachmentCount = 1,
                                                   .pAttachments = &colorBlendAttachments,
                                                   .blendConstants = std::array{ 0.0f, 0.0f, 0.0f, 0.0f } };
    constexpr auto deptStencilStateCreateInfo =
            vk::PipelineDepthStencilStateCreateInfo{ .depthTestEnable = vk::True,
                                                     .depthWriteEnable = vk::True,
                                                     .depthCompareOp = vk::CompareOp::eLess,
                                                     .depthBoundsTestEnable = vk::False,
                                                     .stencilTestEnable = vk::False,
                                                     .front = {},
                                                     .back = {},
                                                     .minDepthBounds = 0.0f,
                                                     .maxDepthBounds = 1.0f };

    return logicalDevice
            .createGraphicsPipelineUnique({},
                                          vk::GraphicsPipelineCreateInfo{
                                                  .pNext = &pipelineRenderingCreateInfo,
                                                  .stageCount = static_cast<uint32_t>(shaderStages.size()),
                                                  .pStages = shaderStages.data(),
                                                  .pVertexInputState = &vertexInputStateCreateInfo,
                                                  .pInputAssemblyState = &inputAssemblyStateCreateInfo,
                                                  .pViewportState = &viewportState,
                                                  .pRasterizationState = &rasterizer,
                                                  .pMultisampleState = &multisampling,
                                                  .pDepthStencilState = &deptStencilStateCreateInfo,
                                                  .pColorBlendState = &colorBlendStateCreateInfo,
                                                  .pDynamicState = &dynamicStateCreateInfo,
                                                  .layout = pipelineLayout,
                                          })
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
    const auto barrier = vk::ImageMemoryBarrier{ .srcAccessMask = srcAccessFlag,
                                                 .dstAccessMask = dstAccessFlag,
                                                 .oldLayout = oldLayout,
                                                 .newLayout = newLayout,
                                                 .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                                                 .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                                                 .image = image,
                                                 .subresourceRange = vk::ImageSubresourceRange{
                                                         .aspectMask = aspectFlags,
                                                         .baseMipLevel = 0,
                                                         .levelCount = mipLevels,
                                                         .baseArrayLayer = 0,
                                                         .layerCount = 1,
                                                 } };
    commandBuffer.pipelineBarrier(srcStages, dstStages, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);
}

void transitImageLayout(const vk::CommandBuffer commandBuffer, const vk::Image image, const vk::ImageLayout oldLayout,
                        const vk::ImageLayout newLayout, const uint32_t mipLevels) {
    const auto layoutTransition = ImageLayoutTransition{ .oldLayout = oldLayout, .newLayout = newLayout };
    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        transitImageLayout(commandBuffer,
                           image,
                           layoutTransition,
                           ImagePipelineStageTransition{ .oldStage = vk::PipelineStageFlagBits::eTopOfPipe,
                                                         .newStage = vk::PipelineStageFlagBits::eTransfer },
                           ImageAccessFlagsTransition{ .oldAccess = vk::AccessFlags(),
                                                       .newAccess = vk::AccessFlagBits::eTransferWrite },
                           vk::ImageAspectFlagBits::eColor,
                           mipLevels);
        return;
    }
    if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        transitImageLayout(commandBuffer,
                           image,
                           layoutTransition,
                           ImagePipelineStageTransition{ .oldStage = vk::PipelineStageFlagBits::eTransfer,
                                                         .newStage = vk::PipelineStageFlagBits::eFragmentShader },
                           ImageAccessFlagsTransition{ .oldAccess = vk::AccessFlagBits::eTransferWrite,
                                                       .newAccess = vk::AccessFlagBits::eShaderRead },
                           vk::ImageAspectFlagBits::eColor,
                           mipLevels);
        return;
    }
    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        transitImageLayout(
                commandBuffer,
                image,
                layoutTransition,
                ImagePipelineStageTransition{ .oldStage = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eNone,
                                              .newStage = vk::PipelineStageFlagBits::eColorAttachmentOutput },
                ImageAccessFlagsTransition{ .oldAccess = vk::AccessFlagBits::eNone,
                                            .newAccess = vk::AccessFlagBits::eColorAttachmentWrite },
                vk::ImageAspectFlagBits::eColor,
                mipLevels);
        return;
    }
    if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal && newLayout == vk::ImageLayout::ePresentSrcKHR) {
        transitImageLayout(commandBuffer,
                           image,
                           layoutTransition,
                           ImagePipelineStageTransition{ .oldStage = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                                         .newStage = vk::PipelineStageFlagBits::eBottomOfPipe },
                           ImageAccessFlagsTransition{ .oldAccess = vk::AccessFlagBits::eColorAttachmentWrite,
                                                       .newAccess = vk::AccessFlagBits() },
                           vk::ImageAspectFlagBits::eColor,
                           mipLevels);
        return;
    }
    transitImageLayout(commandBuffer,
                       image,
                       layoutTransition,
                       ImagePipelineStageTransition{ .oldStage = vk::PipelineStageFlagBits::eAllCommands,
                                                     .newStage = vk::PipelineStageFlagBits::eAllCommands },
                       ImageAccessFlagsTransition{ .oldAccess = vk::AccessFlagBits::eMemoryWrite,
                                                   .newAccess = vk::AccessFlagBits::eMemoryWrite
                                                                | vk::AccessFlagBits::eMemoryRead },
                       vk::ImageAspectFlagBits::eColor,
                       mipLevels);
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