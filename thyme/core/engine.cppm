module;

#include <functional>
#include <string>
#include <variant>

#include <imgui.h>

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

export module th.core.engine;

import th.render_system.vulkan;

import th.platform.glfw.glfw_window;
import th.platform.window;
import th.platform.platform_context;
import th.platform.glfw.glfw_context;

import th.core.events;
import th.core.logger;

import th.scene.model;
import th.scene.camera;

export namespace th::core {

enum struct BackendType {
    Vulkan
};

struct EngineConfig {
    uint32_t width{ 1920 };
    uint32_t height{ 1080 };
    BackendType backend{ BackendType::Vulkan };
    std::string engineName{ "Thyme" };
    std::string appName;
};

class Engine final {
public:
    explicit Engine(const EngineConfig& engineConfig, scene::ModelStorage& modelStorage);

    void run();

private:
    EngineConfig m_engineConfig;
    scene::Camera m_camera;
    scene::ModelStorage& m_modelStorage;
};
}// namespace th::core

module :private;

namespace th::core {
Engine::Engine(const EngineConfig& engineConfig, scene::ModelStorage& modelStorage)
    : m_engineConfig{ engineConfig },
      m_camera{ scene::CameraArguments{ .fov = 45.0f,
                                        .zNear = 0.1f,
                                        .zFar = 100.0f,
                                        .resolution = { engineConfig.width, engineConfig.height },
                                        .eye = { 2.0f, 2.0f, 2.0f },
                                        .center = { 0.0f, 0.0f, 0.0f },
                                        .up = { 0.0f, 0.0f, 1.0f } } },
      m_modelStorage{ modelStorage } {
    [[maybe_unused]] static GlfwVulkanPlatformContext glfwContext;
}

void Engine::run() {
    ThymeLogger::getLogger()->info("Start {} engine", m_engineConfig.engineName);
    WindowResizedEventHandler windowResizedEventHandler;
    windowResizedEventHandler.subscribe([&camera = m_camera](const WindowResize& windowResizedEvent) {
        const auto [width, height] = windowResizedEvent;
        camera.setResolution(glm::vec2{ static_cast<float>(width), static_cast<float>(height) });
    });


    const auto initInfo = VulkanFramework::InitInfo{
        .appName = m_engineConfig.appName,
        .engineName = m_engineConfig.engineName,
    };

    const auto framework = VulkanFramework::create<GlfwVulkanPlatformContext>(initInfo);
    VulkanGlfwWindow window(
            WindowConfig{ .width = m_engineConfig.width, .height = m_engineConfig.height, .name = "Thyme" },
            framework.getInstance());
    window.subscribe([&windowResizedEventHandler](const Event& event) {
        std::visit(
                [&](const auto& windowResizedEvent) {
                    if constexpr (std::is_same_v<std::remove_cvref_t<decltype(windowResizedEvent)>, WindowResize>) {
                        windowResizedEventHandler.next(windowResizedEvent);
                    }
                },
                event);
    });

    const auto physicalDevicesManager = VulkanPhysicalDevicesManager(framework.getInstance(), *window.getSurface());

    const auto& device = physicalDevicesManager.getSelectedDevice();
    const auto swapChainDetails = SwapChainSupportDetails(device.physicalDevice, *window.getSurface());
    const auto graphicContext = VulkanGraphicContext{ .maxFramesInFlight = 2,
                                                        .imageCount = swapChainDetails.getImageCount(),
                                                        .depthFormat = findDepthFormat(device.physicalDevice),
                                                        .colorFormat = vk::Format::eR16G16B16A16Sfloat,
                                                        .surfaceFormat = swapChainDetails.getBestSurfaceFormat(),
                                                        .presentMode = swapChainDetails.getBestPresetMode() };


    Gui gui(device, window, graphicContext, *framework.getInstance());
    auto buffersPool = VulkanCommandBuffersPool(
            device.logicalDevice, device.commandPool, device.getGraphicQueue(), graphicContext.maxFramesInFlight);
    VulkanSwapChain swapChain(device,
                                *window.getSurface(),
                                graphicContext,
                                swapChainDetails.getSwapExtent(window.getFrameBufferSize()),
                                buffersPool);
    VulkanRenderer renderer(device, swapChain, m_modelStorage, m_camera, gui, graphicContext, buffersPool);

    windowResizedEventHandler.subscribe([&swapChain](const WindowResize& windowResized) {
        const auto [width, height] = windowResized;
        swapChain.frameResized(vk::Extent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) });
    });

    while (!window.shouldClose()) {
        window.poolEvents();
        if (!window.isMinimalized()) {
            renderer.draw();
        }
    }

    device.logicalDevice.waitIdle();
}

}// namespace th::core
