export module th.platform.window;

import std;

import th.core.events;
import th.core.logger;

namespace th {

export enum class WindowState {
    minimalized,
    maximalized
};

constexpr std::uint32_t g_defaultWindowWidth{ 1240 };
constexpr std::uint32_t g_defaultWindowHeight{ 620 };
constexpr bool g_windowMaximalizedByDefault{ false };
constexpr bool g_windowDecoratedByDefault{ true };

export struct WindowConfig {
    std::uint32_t width{ g_defaultWindowWidth };
    std::uint32_t height{ g_defaultWindowHeight };
    std::string name{};
    bool maximized{ g_windowMaximalizedByDefault };
    bool decorate{ g_windowDecoratedByDefault };
};

export class Window {
public:
    explicit Window(WindowConfig window_configuration, Logger& logger)
        : config{ std::move(window_configuration) }, m_logger{ logger } {}

    explicit Window(const Window& window) = default;
    explicit Window(Window&& window) = default;

    auto operator=(const Window& window) -> Window& = delete;
    auto operator=(Window&& window) -> Window& = delete;

    virtual void poolEvents() = 0;
    [[nodiscard]] virtual auto shouldClose() -> bool = 0;
    [[nodiscard]] auto isMinimalized() const noexcept -> bool {
        return m_window_state == WindowState::minimalized;
    }

    WindowConfig config;

    auto subscribe(EventSubject::event_fn&& handler) -> int {
        return m_event_listener.subscribe(std::forward<EventSubject::event_fn>(handler));
    }

    virtual ~Window() = default;

protected:
    WindowState m_window_state{ WindowState::maximalized };
    EventSubject m_event_listener{};
    std::reference_wrapper<Logger> m_logger;
};

}// namespace th
