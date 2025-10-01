export module th.platform.window;

import std;

import th.core.events;
import th.core.logger;

namespace th {

export enum class WindowState {
    minimalized,
    maximalized
};

constexpr std::uint32_t defaultWindowWidth{ 1240 };
constexpr std::uint32_t defaultWindowHeight{ 620 };
constexpr bool windowMaximalizedByDefault{ false };
constexpr bool windowDecoratedByDefault{ true };

export struct WindowConfig {
    std::uint32_t width{ defaultWindowWidth };
    std::uint32_t height{ defaultWindowHeight };
    std::string name;
    bool maximalized{ windowMaximalizedByDefault };
    bool decorate{ windowDecoratedByDefault };
};

export class Window {
public:
    explicit Window(WindowConfig windowConfiguration, Logger& logger)
        : config{ std::move(windowConfiguration) }, m_logger{ logger } {}

    explicit Window(const Window& window) = default;
    explicit Window(Window&& window) = default;

    auto operator=(const Window& window) -> Window& = delete;
    auto operator=(Window&& window) -> Window& = delete;

    virtual void poolEvents() = 0;
    [[nodiscard]] virtual auto shouldClose() -> bool = 0;
    [[nodiscard]] auto isMinimalized() const noexcept -> bool {
        return m_windowState == WindowState::minimalized;
    }

    WindowConfig config;

    auto subscribe(EventSubject::event_fn&& handler) -> int {
        return m_eventListener.subscribe(std::forward<EventSubject::event_fn>(handler));
    }

    virtual ~Window() = default;

protected:
    WindowState m_windowState{ WindowState::maximalized };
    EventSubject m_eventListener;
    Logger& m_logger;
};

}// namespace th
