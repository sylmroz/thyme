module;

export module th.platform.window;

import th.core.events;

namespace th {

export struct WindowConfig {
    uint32_t width{ 0 };
    uint32_t height{ 0 };
    std::string name;
};

export enum class WindowState {
    minimalized,
    maximalized
};

export class Window {
public:
    explicit Window(WindowConfig windowConfiguration) : config{ std::move(windowConfiguration) } {}

    explicit Window(const Window& window) = default;
    explicit Window(Window&& window) = default;

    auto operator=(const Window& window) -> Window& = default;
    auto operator=(Window&& window) -> Window& = default;

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
};

}// namespace th
