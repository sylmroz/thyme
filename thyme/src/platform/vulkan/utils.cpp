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

static constexpr auto deviceExtensions =
        std::array{ vk::KHRSwapchainExtensionName, vk::KHRDynamicRenderingExtensionName };

bool deviceHasAllRequiredExtensions(const vk::PhysicalDevice& physicalDevice) {
    const auto& availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    return std::ranges::all_of(deviceExtensions, [&availableDeviceExtensions](const auto& extension) {
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

QueueFamilyIndices::QueueFamilyIndices(const vk::PhysicalDevice& device, const vk::SurfaceKHR surface) noexcept {
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

std::vector<PhysicalDevice> getPhysicalDevices(const vk::UniqueInstance& instance, const vk::SurfaceKHR surface) {
    static std::map<vk::PhysicalDeviceType, uint32_t> priorities = {
        { vk::PhysicalDeviceType::eOther, 0 },       { vk::PhysicalDeviceType::eCpu, 1 },
        { vk::PhysicalDeviceType::eVirtualGpu, 2 },  { vk::PhysicalDeviceType::eIntegratedGpu, 3 },
        { vk::PhysicalDeviceType::eDiscreteGpu, 4 },
    };

    std::vector<PhysicalDevice> physicalDevices;
    for (const auto& device : instance->enumeratePhysicalDevices()) {
        const auto queueFamilyIndex = QueueFamilyIndices(device, surface);
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

[[nodiscard]] vk::UniqueDevice PhysicalDevice::createLogicalDevice() const {
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
                             const vk::RenderPass renderPass,
                             const vk::SurfaceKHR surface,
                             const vk::ImageView colorImageView,
                             const vk::ImageView depthImageView,
                             const vk::SwapchainKHR oldSwapChain) {
    const auto& [surfaceFormat, presetMode, imageCount] = swapChainSettings;
    [[maybe_unused]] const auto& [physicalDevice,
                                  logicalDevice,
                                  queueFamilyIndices,
                                  swapChainSupportDetails,
                                  maxMsaaSamples] = device;
    const auto swapChainCreateInfo = [&] {
        auto info = vk::SwapchainCreateInfoKHR(vk::SwapchainCreateFlagsKHR(),
                                               surface,
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
        if (queueFamilyIndices.graphicFamily.value() != queueFamilyIndices.presentFamily.value()) {
            const auto indices =
                    std::array{ queueFamilyIndices.graphicFamily.value(), queueFamilyIndices.presentFamily.value() };
            info.imageSharingMode = vk::SharingMode::eConcurrent;
            info.setQueueFamilyIndices(indices);
        } else {
            info.imageSharingMode = vk::SharingMode::eExclusive;
        }
        info.oldSwapchain = oldSwapChain;
        return info;
    }();

    swapChain = logicalDevice->createSwapchainKHRUnique(swapChainCreateInfo);
    swapChainFrame = logicalDevice->getSwapchainImagesKHR(swapChain.get())
                     | std::views::transform([&](vk::Image image) -> SwapChainFrame {
                           auto imageView = logicalDevice->createImageViewUnique(vk::ImageViewCreateInfo(
                                   vk::ImageViewCreateFlags(),
                                   image,
                                   vk::ImageViewType::e2D,
                                   surfaceFormat.format,
                                   vk::ComponentMapping(),
                                   vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
                           const auto attachments = std::array{ colorImageView, depthImageView, imageView.get() };
                           auto frameBuffer = logicalDevice->createFramebufferUnique(
                                   vk::FramebufferCreateInfo(vk::FramebufferCreateFlagBits(),
                                                             renderPass,
                                                             attachments,
                                                             swapChainExtent.width,
                                                             swapChainExtent.height,
                                                             1));
                           return SwapChainFrame{ std::move(image), std::move(imageView), std::move(frameBuffer) };
                       })
                     | std::ranges::to<decltype(swapChainFrame)>();
}


FrameDataList::FrameDataList(const vk::Device logicalDevice, const vk::CommandPool commandPool,
                             const uint32_t maxFrames) noexcept {
    m_frameDataList.reserve(maxFrames);
    for (uint32_t i = 0; i < maxFrames; i++) {
        m_frameDataList.emplace_back(FrameData{
                .commandBuffer = std::move(logicalDevice
                                                   .allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
                                                           commandPool, vk::CommandBufferLevel::ePrimary, 1))
                                                   .front()),
                .imageAvailableSemaphore = logicalDevice.createSemaphoreUnique(vk::SemaphoreCreateInfo()),
                .renderFinishedSemaphore = logicalDevice.createSemaphoreUnique(vk::SemaphoreCreateInfo()),
                .fence = logicalDevice.createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)),
                .currentFrame = i,
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
    const auto& [logicalDevice, renderPass, pipelineLayout, samples, shaderStages] = graphicPipelineCreateInfo;
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
                                                                         renderPass,
                                                                         0))
            .value;
}

BufferMemory::BufferMemory(const Device& device, const size_t size, const vk::BufferUsageFlags usage,
                           const vk::MemoryPropertyFlags properties) {
    m_buffer = device.logicalDevice->createBufferUnique(
            vk::BufferCreateInfo(vk::BufferCreateFlagBits(), size, usage, vk::SharingMode::eExclusive));

    vk::MemoryRequirements memoryRequirements;
    device.logicalDevice->getBufferMemoryRequirements(m_buffer.get(), &memoryRequirements);

    m_memory = device.logicalDevice->allocateMemoryUnique(vk::MemoryAllocateInfo(
            memoryRequirements.size,
            findMemoryType(device.physicalDevice, memoryRequirements.memoryTypeBits, properties)));

    device.logicalDevice->bindBufferMemory(m_buffer.get(), m_memory.get(), 0);
}

void transitImageLayout(const Device& device, const vk::CommandPool commandPool, const vk::Image image,
                        const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout, const uint32_t mipLevels) {
    singleTimeCommand(device, commandPool, [&](const vk::CommandBuffer commandBuffer) {
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
            if (oldLayout == vk::ImageLayout::eTransferDstOptimal
                && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
                return std::tuple(createBarrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead),
                                  vk::PipelineStageFlagBits::eTransfer,
                                  vk::PipelineStageFlagBits::eFragmentShader);
            }
            throw std::runtime_error("failed to transit image layout!");
        }();
        commandBuffer.pipelineBarrier(
                srcPipelineStage, dstPipelineStage, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);
    });
}

ImageMemory::ImageMemory(const Device& device, const Resolution resolution, const vk::Format format,
                         const vk::ImageUsageFlags imageUsageFlags, const vk::MemoryPropertyFlags memoryPropertyFlags,
                         const vk::ImageAspectFlags aspectFlags, const vk::SampleCountFlagBits msaa,
                         const uint32_t mipLevels) {
    m_image = device.logicalDevice->createImageUnique(
            vk::ImageCreateInfo(vk::ImageCreateFlags(),
                                vk::ImageType::e2D,
                                format,
                                vk::Extent3D(resolution.width, resolution.height, 1),
                                mipLevels,
                                1,
                                msaa,
                                vk::ImageTiling::eOptimal,
                                imageUsageFlags,
                                vk::SharingMode::eExclusive));
    vk::MemoryRequirements memoryRequirements;
    device.logicalDevice->getImageMemoryRequirements(m_image.get(), &memoryRequirements);
    m_memory = device.logicalDevice->allocateMemoryUnique(vk::MemoryAllocateInfo(
            memoryRequirements.size,
            findMemoryType(device.physicalDevice, memoryRequirements.memoryTypeBits, memoryPropertyFlags)));
    device.logicalDevice->bindImageMemory(m_image.get(), m_memory.get(), 0);
    m_imageView = device.logicalDevice->createImageViewUnique(
            vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(),
                                    m_image.get(),
                                    vk::ImageViewType::e2D,
                                    format,
                                    vk::ComponentMapping(),
                                    vk::ImageSubresourceRange(aspectFlags, 0, mipLevels, 0, 1)));
}

void generateMipmaps(const Device& device, const vk::CommandPool commandPool, const vk::Image image,
                     const vk::Format format, const Resolution resolution, const uint32_t mipLevels) {
    if (!(device.physicalDevice.getFormatProperties(format).optimalTilingFeatures
          & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("Failed to generate mipmaps! Unsupported linear filtering!");
    }
    singleTimeCommand(device, commandPool, [&](const vk::CommandBuffer& commandBuffer) {
        const auto getImageBarrier = [&image](const uint32_t mipLevel) -> vk::ImageMemoryBarrier {
            return vk::ImageMemoryBarrier(
                    vk::AccessFlagBits::eTransferWrite,
                    vk::AccessFlagBits::eTransferRead,
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::ImageLayout::eTransferSrcOptimal,
                    vk::QueueFamilyIgnored,
                    vk::QueueFamilyIgnored,
                    image,
                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, mipLevel, 1, 0, 1));
        };
        const auto getImageBlit =
                [](const uint32_t mipLevel, const int mipWidth, const int mipHeight) -> vk::ImageBlit {
            return vk::ImageBlit(
                    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, mipLevel, 0, 1),
                    std::array{ vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ mipWidth, mipHeight, 1 } },
                    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, mipLevel + 1, 0, 1),
                    std::array{
                            vk::Offset3D{ 0, 0, 0 },
                            vk::Offset3D{ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 } });
        };
        int mipWidth = static_cast<int>(resolution.width), mipHeight = static_cast<int>(resolution.height);
        for (uint32_t mipLevel = 0; mipLevel < mipLevels - 1; ++mipLevel, mipWidth /= 2, mipHeight /= 2) {
            auto barrier = getImageBarrier(mipLevel);
            commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                          vk::PipelineStageFlagBits::eTransfer,
                                          vk::DependencyFlags(),
                                          {},
                                          {},
                                          { barrier });

            const auto blit = getImageBlit(mipLevel, mipWidth, mipHeight);
            commandBuffer.blitImage(image,
                                    vk::ImageLayout::eTransferSrcOptimal,
                                    image,
                                    vk::ImageLayout::eTransferDstOptimal,
                                    { blit },
                                    vk::Filter::eLinear);
            barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                          vk::PipelineStageFlagBits::eFragmentShader,
                                          vk::DependencyFlags(),
                                          {},
                                          {},
                                          { barrier });
        }
        auto barrier = getImageBarrier(mipLevels - 1);
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                      vk::PipelineStageFlagBits::eFragmentShader,
                                      vk::DependencyFlags(),
                                      {},
                                      {},
                                      { barrier });
    });
}

ImageMemory::ImageMemory(const Device& device, const vk::CommandPool commandPool, const std::span<const uint8_t> data,
                         const Resolution resolution, const vk::SampleCountFlagBits msaa, const uint32_t mipLevels)
    : ImageMemory(device,
                  resolution,
                  vk::Format::eR8G8B8A8Srgb,
                  vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
                          | vk::ImageUsageFlagBits::eSampled,
                  vk::MemoryPropertyFlagBits::eDeviceLocal,
                  vk::ImageAspectFlagBits::eColor,
                  msaa,
                  mipLevels) {
    const auto stagingMemoryBuffer =
            BufferMemory(device,
                         data.size(),
                         vk::BufferUsageFlagBits::eTransferSrc,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    void* mappedMemory = nullptr;
    [[maybe_unused]] const auto result = device.logicalDevice->mapMemory(
            stagingMemoryBuffer.getMemory().get(), 0, data.size(), vk::MemoryMapFlags(), &mappedMemory);
    memcpy(mappedMemory, data.data(), data.size());
    device.logicalDevice->unmapMemory(stagingMemoryBuffer.getMemory().get());

    transitImageLayout(device,
                       commandPool,
                       m_image.get(),
                       vk::ImageLayout::eUndefined,
                       vk::ImageLayout::eTransferDstOptimal,
                       mipLevels);
    copyBufferToImage(device, commandPool, stagingMemoryBuffer.getBuffer().get(), m_image.get(), resolution);
    generateMipmaps(device, commandPool, m_image.get(), vk::Format::eR8G8B8A8Srgb, resolution, mipLevels);
}

auto findSupportedImageFormat(const vk::PhysicalDevice& device, const std::span<const vk::Format> formats,
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

}// namespace th::vulkan