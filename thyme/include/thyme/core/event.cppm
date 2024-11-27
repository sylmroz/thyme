module;

#include <thyme/export_macros.hpp>

#include <glm/vec2.hpp>

#include <functional>
#include <variant>

export module thyme.core.event;
import thyme.core.key_codes;
import thyme.core.mouse_codes;
import thyme.core.utils;

export namespace Thyme {

struct THYME_API WindowResize {
    int width;
    int height;

    std::string toString() const {
        return std::format("WindowResize {{ width:{} height:{} }}", width, height);
    }
};

struct THYME_API WindowClose {};

struct THYME_API MouseMove {
    glm::vec2 position;
};

struct THYME_API MouseButtonDown {
    MouseButton button;
};

struct THYME_API MouseButtonUp {
    MouseButton button;
};

struct THYME_API MouseWheel {
    int wheel;
};

struct THYME_API KeyPressed {
    KeyCode code;
};

struct THYME_API KeyPressedRepeated {
    KeyCode code;
};

struct THYME_API KeyReleased {
    KeyCode code;
};

using WindowEvent = std::variant<WindowResize, WindowClose>;

using MouseEvent = std::variant<MouseMove, MouseButtonDown, MouseButtonUp>;

using KeyEvent = std::variant<KeyPressed, KeyReleased>;

using Event = std::variant<WindowResize,
                           WindowClose,
                           MouseMove,
                           MouseButtonDown,
                           MouseButtonUp,
                           KeyPressed,
                           KeyPressedRepeated,
                           KeyReleased>;

class THYME_API EventSubject {
public:
    using event_fn = std::function<void(Event)>;

    void next(const Event& event) const noexcept {
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
    std::vector<std::pair<event_fn, int>> m_handlers;
    int m_handlerId{ 0 };
};

template <typename Event>
struct EventDispatcherHelper : NoCopyable {
    virtual void operator()(const Event& event) = 0;
};

template <typename... Events>
struct EventDispatcher : EventDispatcherHelper<Events>... {
    void operator()(auto&&) {}
};

}// namespace Thyme