#pragma once

#include <thyme/export_macros.hpp>

#include <string>
#include <utility>

#include <thyme/core/engine.hpp>
#include <thyme/core/event.hpp>

namespace th {

struct THYME_API WindowConfig {
    WindowConfig() = default;
    explicit WindowConfig(const EngineConfig& config, const EventSubject& eventSubject)
        : width{ config.width }, height{ config.height }, name{ config.appName }, eventSubject{ eventSubject } {}
    uint32_t width{ 0 };
    uint32_t height{ 0 };
    std::string name;
    EventSubject eventSubject;
};

enum class WindowState {
    minimalized,
    maximalized
};

class THYME_API Window {
public:
    explicit Window(WindowConfig windowConfiguration) : config{ std::move(windowConfiguration) } {}

    explicit Window(const Window& window) = default;
    explicit Window(Window&& window) = default;

    Window& operator=(const Window& window) = default;
    Window& operator=(Window&& window) = default;

    virtual void poolEvents() = 0;
    [[nodiscard]] virtual bool shouldClose() = 0;
    [[nodiscard]] inline bool isMinimalized() const noexcept {
        return m_windowState == WindowState::minimalized;
    }

    WindowConfig config;

    virtual ~Window() = default;
protected: 
    WindowState m_windowState{ WindowState::maximalized };
};

}// namespace Thyme