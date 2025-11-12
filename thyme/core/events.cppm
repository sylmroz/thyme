module;

export module th.core.events;

import std;

import glm;

import th.core.key_codes;
import th.core.utils;
import th.core.mouse_codes;

export namespace th {

struct WindowResizedEvent {
    int width;
    int height;

    [[nodiscard]] auto toString() const -> std::string {
        return std::format("WindowResize {{ width:{} height:{} }}", width, height);
    }
};

struct WindowClosedEvent {};

struct WindowMinimalizedEvent {
    bool minimized;
};

struct WindowMaximizedEvent {
    bool maximized;
};

struct MousePositionEvent {
    glm::vec2 pos;
    [[nodiscard]] auto toString() const -> std::string {
        return std::format("MousePosition {{ x: {}, y: {}}}", pos.x, pos.y);
    }
};

struct MouseButtonPressedEvent {
    MouseButton button;
};

struct MouseButtonReleasedEvent {
    MouseButton button;
};

struct MouseWheelEvent {
    glm::vec2 wheel;
};

struct KeyPressedEvent {
    KeyCode code;
};

struct KeyRepeatedEvent {
    KeyCode code;
};

struct KeyReleasedEvent {
    KeyCode code;
};

using Event = std::variant<WindowResizedEvent,
                           WindowClosedEvent,
                           WindowMinimalizedEvent,
                           WindowMaximizedEvent,
                           MousePositionEvent,
                           MouseWheelEvent,
                           MouseButtonPressedEvent,
                           MouseButtonReleasedEvent,
                           KeyPressedEvent,
                           KeyReleasedEvent,
                           KeyRepeatedEvent>;

template <typename EventType, typename Dispatcher>
    requires(std::is_invocable_v<Dispatcher, EventType>)
class GenericEventHandler {
public:
    void invoke(const EventType& event) const noexcept {
        for (const auto& f : m_handlers | std::views::keys) {
            f(event);
        }
    }

    auto subscribe(Dispatcher&& fn) noexcept -> uint32_t {
        m_handlers.emplace_back(std::make_pair(std::forward<Dispatcher>(fn), m_handler_id));
        return m_handler_id++;
    }

    void unsubscribe(const uint32_t id) noexcept {
        if (const auto it = std::ranges::find(m_handlers, id, &std::pair<Dispatcher, uint32_t>::second);
            it != m_handlers.end()) {
            m_handlers.erase(it);
        }
    }

private:
    uint32_t m_handler_id{ 0 };
    std::vector<std::pair<Dispatcher, uint32_t>> m_handlers;
};

template <typename T>
struct crtp {
    auto underlying() -> T& {
        return static_cast<T&>(*this);
    }
    auto underlying() const -> T const& {
        return static_cast<T const&>(*this);
    }
};

template <typename EventDispatcherType>
struct EventDispatcher: crtp<EventDispatcherType> {
    void operator()(const Event& event) const {
        std::visit(this->underlying(), event);
    }
    void operator()(const auto&) const {}
};

template <typename... Ts>
struct Overload: Ts... {
    using Ts::operator()...;
};

template <class... Ts>
Overload(Ts...) -> Overload<Ts...>;

template <typename EventType>
using EventTypeFunctionDispatcher = GenericEventHandler<EventType, std::function<void(EventType)>>;

using WindowResizedEventHandler = EventTypeFunctionDispatcher<WindowResizedEvent>;
using WindowClosedEventHandler = EventTypeFunctionDispatcher<WindowClosedEvent>;
using WindowMinimalizedEventHandler = EventTypeFunctionDispatcher<WindowMinimalizedEvent>;
using WindowMaximalizedEventHandler = EventTypeFunctionDispatcher<WindowMaximizedEvent>;

using MousePositionEventHandler = EventTypeFunctionDispatcher<MousePositionEvent>;
using MouseWheelEventHandler = EventTypeFunctionDispatcher<MouseWheelEvent>;
using MouseButtonPressedEventHandler = EventTypeFunctionDispatcher<MouseButtonPressedEvent>;
using MouseButtonReleasedEventHandler = EventTypeFunctionDispatcher<MouseButtonReleasedEvent>;

using KeyPressedEventHandler = EventTypeFunctionDispatcher<KeyPressedEvent>;
using KeyRepeatedEventHandler = EventTypeFunctionDispatcher<KeyRepeatedEvent>;
using KeyReleasedEventHandler = EventTypeFunctionDispatcher<KeyReleasedEvent>;

}// namespace th

template <>
struct std::formatter<th::WindowResizedEvent>: std::formatter<std::string> {
    auto format(th::WindowResizedEvent window_resize, std::format_context& ctx) const {
        return formatter<std::string>::format(
                std::format("{{ width: {}, height: {} }}", window_resize.width, window_resize.height), ctx);
    }
};

template <>
struct std::formatter<th::MousePositionEvent>: std::formatter<std::string> {
    auto format(th::MousePositionEvent mouse_position, std::format_context& ctx) const {
        return formatter<std::string>::format(
                std::format("{{ x: {}, y: {} }}", mouse_position.pos.x, mouse_position.pos.y), ctx);
    }
};
