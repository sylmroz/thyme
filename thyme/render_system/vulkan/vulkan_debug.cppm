module;
#if !defined(NDEBUG)
#include <vulkan/vk_platform.h>

export module th.render_system.vulkan:debug;

import std;

import vulkan;

import th.core.logger;

namespace th {

using namespace std::string_view_literals;

VKAPI_ATTR auto VKAPI_CALL debugCallback(const vk::DebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                         const vk::DebugUtilsMessageTypeFlagsEXT message_types,
                                         const vk::DebugUtilsMessengerCallbackDataEXT* callback_data,
                                         void* user_data) -> vk::Bool32;

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
        constexpr auto type_flags = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                                   | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
                                   | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
        return vk::DebugUtilsMessengerCreateInfoEXT{
            .messageSeverity = flags,
            .messageType = type_flags,
            .pfnUserCallback = debugCallback,
            .pUserData = logger,
        };
    }


private:
    vk::raii::DebugUtilsMessengerEXT m_debugMessenger;
};

VKAPI_ATTR auto VKAPI_CALL debugCallback(const vk::DebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                         const vk::DebugUtilsMessageTypeFlagsEXT message_types,
                                         const vk::DebugUtilsMessengerCallbackDataEXT* callback_data,
                                         void* user_data) -> vk::Bool32 {
    std::string message_details;
    if (const auto queue_labels = std::span(callback_data->pQueueLabels, callback_data->queueLabelCount);
        !queue_labels.empty()) {
        message_details += "\n\tQueue labels:";
        for (const auto queueLabel : queue_labels) {
            message_details += std::format("\n\t\tLabel name: {}", queueLabel.pLabelName);
        }
    }
    if (const auto cmd_buf_labels = std::span(callback_data->pCmdBufLabels, callback_data->cmdBufLabelCount);
        !cmd_buf_labels.empty()) {
        message_details += "\n\tCommand buffer labels:";
        for (const auto cmd_buffer_label : cmd_buf_labels) {
            message_details += std::format("\n\t\tLabel name: {}", cmd_buffer_label.pLabelName);
        }
    }
    if (const auto objects = std::span(callback_data->pObjects, callback_data->objectCount); !objects.empty()) {
        message_details += "\n\tObjects:";
        for (const auto object : objects) {
            if (object.pObjectName != nullptr) {
                message_details += std::format("\n\t\tObject name: {}, Object type: {}, Object handle: {}",
                                               object.pObjectName,
                                               vk::to_string(object.objectType),
                                               object.objectHandle);
            } else {
                message_details += std::format("\n\t\tObject type: {}, Object handle: {}",
                                               vk::to_string(object.objectType),
                                               object.objectHandle);
            }
        }
    }

    const auto message_type_str = vk::to_string(message_types);
    const auto message = std::format("[{}]: Name: {}, Message: {}{}",
                                     message_type_str,
                                     callback_data->pMessageIdName,
                                     callback_data->pMessage,
                                     message_details);
    const auto logger = static_cast<Logger*>(user_data);
    switch (message_severity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose: logger->trace("{}", message); break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo: logger->info("{}", message); break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning: logger->warn("{}", message); break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError: logger->error("{}", message); break;
        default: break;
    }
    return false;
}

}// namespace th
#endif
