module;
#if !defined(NDEBUG)
#include <vulkan/vk_platform.h>

#include <format>
#include <span>

export module th.render_system.vulkan:debug;

import vulkan_hpp;

import th.core.logger;

namespace th {

using namespace std::string_view_literals;

VKAPI_ATTR auto VKAPI_CALL debugCallback(const vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                         const vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
                                         const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                         void* pUserData) -> vk::Bool32;

export class VulkanDebug {
public:
    explicit VulkanDebug(const vk::raii::Instance& instance, Logger& logger)
        : m_debugMessenger{ instance.createDebugUtilsMessengerEXT(createDebugUtilsMessengerCreateInfo(&logger)) } {}

    VulkanDebug(const VulkanDebug&) = delete;
    VulkanDebug(VulkanDebug&&) = delete;
    auto operator=(const VulkanDebug&) -> VulkanDebug& = delete;
    auto operator=(VulkanDebug&&) -> VulkanDebug& = delete;
    ~VulkanDebug() = default;

    static auto createDebugUtilsMessengerCreateInfo(Logger* logger) -> vk::DebugUtilsMessengerCreateInfoEXT {
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
            .pUserData = logger,
        };
    }


private:
    vk::raii::DebugUtilsMessengerEXT m_debugMessenger;
};

VKAPI_ATTR auto VKAPI_CALL debugCallback(const vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                         const vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
                                         const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                         void* pUserData) -> vk::Bool32 {
    std::string messageDetails;
    if (const auto queueLabels = std::span(pCallbackData->pQueueLabels, pCallbackData->queueLabelCount);
        !queueLabels.empty()) {
        messageDetails += "\n\tQueue labels:";
        for (const auto queueLabel : queueLabels) {
            messageDetails += std::format("\n\t\tLabel name: {}", queueLabel.pLabelName);
        }
    }
    if (const auto cmdBufLabels = std::span(pCallbackData->pCmdBufLabels, pCallbackData->cmdBufLabelCount);
        !cmdBufLabels.empty()) {
        messageDetails += "\n\tCommand buffer labels:";
        for (const auto cmdBufferLabel : cmdBufLabels) {
            messageDetails += std::format("\n\t\tLabel name: {}", cmdBufferLabel.pLabelName);
        }
    }
    if (const auto objects = std::span(pCallbackData->pObjects, pCallbackData->objectCount); !objects.empty()) {
        messageDetails += "\n\tObjects:";
        for (const auto object : objects) {
            if (object.pObjectName != nullptr) {
                messageDetails += std::format("\n\t\tObject name: {}, Object type: {}, Object handle: {}",
                                              object.pObjectName,
                                              vk::to_string(object.objectType),
                                              object.objectHandle);
            } else {
                messageDetails += std::format("\n\t\tObject type: {}, Object handle: {}",
                                              vk::to_string(object.objectType),
                                              object.objectHandle);
            }
        }
    }

    const auto messageTypeStr = vk::to_string(messageTypes);
    const auto message = std::format("[{}]: Name: {}, Message: {}{}",
                                     messageTypeStr,
                                     pCallbackData->pMessageIdName,
                                     pCallbackData->pMessage,
                                     messageDetails);
    const auto logger = static_cast<Logger*>(pUserData);
    switch (messageSeverity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose: logger->trace("{}", message); break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo: logger->info("{}", message); break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning: logger->warn("{}", message); break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError: logger->error("{}", message); break;
        default: break;
    }
    return false;
}

}// namespace th::render_system::vulkan
#endif
