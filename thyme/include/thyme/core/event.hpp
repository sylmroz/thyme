#pragma once

#include <thyme/export_macros.hpp>

#include <fmt/format.h>
#include <glm/vec2.hpp>

#include <functional>
#include <ranges>
#include <variant>

#include <thyme/core/key_codes.hpp>
#include <thyme/core/mouse_codes.hpp>
#include <thyme/core/utils.hpp>

namespace th {

struct THYME_API WindowResize {
    int width;
    int height;

    std::string toString() const {
        return fmt::format("WindowResize {{ width:{} height:{} }}", width, height);
    }
};

struct THYME_API WindowClose{};

struct THYME_API WindowMinimalize{};

struct THYME_API WindowMaximalize{};

struct THYME_API MousePosition {
    glm::vec2 pos;
    auto toString() const -> std::string {
        return fmt::format("MousePosition {{ x: {}, y: {}}}", pos.x, pos.y);
    }
};

struct THYME_API MouseButtonPress {
    MouseButton button;
};

struct THYME_API MouseButtonReleased {
    MouseButton button;
};

struct THYME_API MouseWheel {
    glm::vec2 wheel;
};

struct THYME_API KeyPressed {
    KeyCode code;
};

struct THYME_API KeyRepeated {
    KeyCode code;
};

struct THYME_API KeyReleased {
    KeyCode code;
};

using WindowEvent = std::variant<WindowResize, WindowClose, WindowMinimalize, WindowMaximalize>;

using MouseEvent = std::variant<MousePosition, MouseWheel, MouseButtonPress, MouseButtonReleased>;

using KeyEvent = std::variant<KeyPressed, KeyReleased, KeyRepeated>;

using Event = std::variant<WindowResize,
                           WindowClose,
                           WindowMinimalize,
                           WindowMaximalize,
                           MousePosition,
                           MouseWheel,
                           MouseButtonPress,
                           MouseButtonReleased,
                           KeyPressed,
                           KeyRepeated,
                           KeyReleased>;

template <typename EventType>
class THYME_API EventHandler {
    using event_fn = std::function<void(EventType)>;

public:
    void next(const EventType& event) const noexcept {
        for (const auto& f : m_handlers | std::views::keys) {
            f(event);
        }
    }

    int subscribe(event_fn fn) noexcept {
        m_handlers.emplace_back(std::make_pair(fn, m_handlerId));
        return m_handlerId++;
    }

    void unsubscribe(const int id) noexcept {
        if (const auto it = std::ranges::find(m_handlers, id, &std::pair<event_fn, int>::second);
            it != m_handlers.end()) {
            m_handlers.erase(it);
        }
    }

private:
    int m_handlerId{ 0 };
    std::vector<std::pair<event_fn, int>> m_handlers;
};

using EventSubject = EventHandler<Event>;
using WindowResizedEventHandler = EventHandler<WindowResize>;
using WindowClosedEventHandler = EventHandler<WindowClose>;
using WindowMinimalizedEventHandler = EventHandler<WindowMinimalize>;
using WindowMaximalizedEventHandler = EventHandler<WindowMaximalize>;
using MousePositionEventHandler = EventHandler<MousePosition>;
using MouseWheelEventHandler = EventHandler<MouseWheel>;
using MouseButtonDownEventHandler = EventHandler<MouseButtonPress>;
using MouseButtonUpEventHandler = EventHandler<MouseButtonReleased>;
using KeyPressedEventHandler = EventHandler<KeyPressed>;
using KeyRepeatedEventHandler = EventHandler<KeyRepeated>;

template <typename Event>
struct EventDispatcherHelper: NoCopyable {
    EventDispatcherHelper() = default;
    virtual void operator()(const Event& event) = 0;
};

template <typename... Events>
struct EventDispatcher: EventDispatcherHelper<Events>... {
    void operator()(auto&&) {}
};

}// namespace th