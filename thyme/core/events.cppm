module;

export module th.core.events;

import std;

import glm;

import th.core.key_codes;
import th.core.utils;
import th.core.mouse_codes;

export namespace th {

struct WindowResize {
    int width;
    int height;

    [[nodiscard]] auto toString() const -> std::string {
        return std::format("WindowResize {{ width:{} height:{} }}", width, height);
    }
};

struct WindowClose {};

struct WindowMinimalize {
    bool minimized;
};

struct WindowMaximize {
    bool maximized;
};

struct MousePosition {
    glm::vec2 pos;
    [[nodiscard]] auto toString() const -> std::string {
        return std::format("MousePosition {{ x: {}, y: {}}}", pos.x, pos.y);
    }
};

struct MouseButtonPress {
    MouseButton button;
};

struct MouseButtonReleased {
    MouseButton button;
};

struct MouseWheel {
    glm::vec2 wheel;
};

struct KeyPressed {
    KeyCode code;
};

struct KeyRepeated {
    KeyCode code;
};

struct KeyReleased {
    KeyCode code;
};

using WindowEvent = std::variant<WindowResize, WindowClose, WindowMinimalize, WindowMaximize>;

using MouseEvent = std::variant<MousePosition, MouseWheel, MouseButtonPress, MouseButtonReleased>;

using KeyEvent = std::variant<KeyPressed, KeyReleased, KeyRepeated>;

using Event = std::variant<WindowResize,
                           WindowClose,
                           WindowMinimalize,
                           WindowMaximize,
                           MousePosition,
                           MouseWheel,
                           MouseButtonPress,
                           MouseButtonReleased,
                           KeyPressed,
                           KeyRepeated,
                           KeyReleased>;

template <typename EventType>
class EventHandler {
public:
    using event_fn = std::function<void(EventType)>;
    void next(const EventType& event) const noexcept {
        for (const auto& f : m_handlers | std::views::keys) {
            f(event);
        }
    }

    auto subscribe(event_fn&& fn) noexcept -> int {
        m_handlers.emplace_back(std::make_pair(std::forward<event_fn>(fn), m_handler_id));
        return m_handler_id++;
    }

    void unsubscribe(const int id) noexcept {
        if (const auto it = std::ranges::find(m_handlers, id, &std::pair<event_fn, int>::second);
            it != m_handlers.end()) {
            m_handlers.erase(it);
        }
    }

private:
    int m_handler_id{ 0 };
    std::vector<std::pair<event_fn, int>> m_handlers;
};

using EventSubject = EventHandler<Event>;
using WindowResizedEventHandler = EventHandler<WindowResize>;
using WindowClosedEventHandler = EventHandler<WindowClose>;
using WindowMinimalizedEventHandler = EventHandler<WindowMinimalize>;
using WindowMaximalizedEventHandler = EventHandler<WindowMaximize>;
using MousePositionEventHandler = EventHandler<MousePosition>;
using MouseWheelEventHandler = EventHandler<MouseWheel>;
using MouseButtonDownEventHandler = EventHandler<MouseButtonPress>;
using MouseButtonUpEventHandler = EventHandler<MouseButtonReleased>;
using KeyPressedEventHandler = EventHandler<KeyPressed>;
using KeyRepeatedEventHandler = EventHandler<KeyRepeated>;

template <typename Event>
struct EventDispatcherHelper {
    EventDispatcherHelper() = default;
    virtual void operator()(const Event& event) = 0;

    EventDispatcherHelper(const EventDispatcherHelper&) = default;
    EventDispatcherHelper(EventDispatcherHelper&&) = default;
    auto operator=(const EventDispatcherHelper&) -> EventDispatcherHelper& = default;
    auto operator=(EventDispatcherHelper&&) -> EventDispatcherHelper& = default;

    virtual ~EventDispatcherHelper() = default;
};

template <typename T>
struct crtp
{
    auto underlying() -> T& { return static_cast<T&>(*this); }
    auto underlying() const -> T const& { return static_cast<T const&>(*this); }
};

template <typename EventDispatcherType>
struct EventDispatcher : crtp<EventDispatcherType> {
    void operator()(const Event& event) const {
        std::visit( this->underlying(), event);
    }
    void operator()(const auto&) const {}
};

template <typename... Ts>
struct Overload: Ts... {
    using Ts::operator()...;
};

template <class... Ts>
Overload(Ts...) -> Overload<Ts...>;

}// namespace th

template <>
struct std::formatter<th::WindowResize>: std::formatter<std::string> {
    auto format(th::WindowResize window_resize, std::format_context& ctx) const {
        return formatter<std::string>::format(
                std::format("{{ width: {}, height: {} }}", window_resize.width, window_resize.height), ctx);
    }
};

template <>
struct std::formatter<th::MousePosition>: std::formatter<std::string> {
    auto format(th::MousePosition mouse_position, std::format_context& ctx) const {
        return formatter<std::string>::format(
                std::format("{{ x: {}, y: {} }}", mouse_position.pos.x, mouse_position.pos.y), ctx);
    }
};
