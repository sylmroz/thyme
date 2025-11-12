export module th.platform.window_event_handler;

import std;

import th.core.events;
import th.core.utils;

namespace th {
template <typename AnyWindowEventType>
concept is_any_window_type_event = either<AnyWindowEventType,
                                          WindowResizedEvent,
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

export class WindowEventsHandlers {
public:
    template <typename EventType>
        requires(is_any_window_type_event<std::remove_cvref_t<EventType>>)
    auto addEventListener(std::function<void(EventType)>&& f) -> void {
        using event_type = std::remove_cvref_t<EventType>;
        if constexpr (std::is_same_v<event_type, WindowResizedEvent>) {
            m_window_resized_handler.subscribe(std::forward<decltype(f)>(f));
        } else if constexpr (std::is_same_v<event_type, WindowClosedEvent>) {
            m_window_closed_handler.subscribe(std::forward<decltype(f)>(f));
        } else if constexpr (std::is_same_v<event_type, WindowMaximizedEvent>) {
            m_window_maximized_handler.subscribe(std::forward<decltype(f)>(f));
        } else if constexpr (std::is_same_v<event_type, WindowMinimalizedEvent>) {
            m_window_minimized_handler.subscribe(std::forward<decltype(f)>(f));
        } else if constexpr (std::is_same_v<event_type, MousePositionEvent>) {
            m_mouse_position_event_handler.subscribe(std::forward<decltype(f)>(f));
        } else if constexpr (std::is_same_v<event_type, MouseWheelEvent>) {
            m_mouse_wheel_event_handler.subscribe(std::forward<decltype(f)>(f));
        } else if constexpr (std::is_same_v<event_type, MouseButtonPressedEvent>) {
            m_mouse_button_pressed_handler.subscribe(std::forward<decltype(f)>(f));
        } else if constexpr (std::is_same_v<event_type, MouseButtonReleasedEvent>) {
            m_mouse_button_released_event_handler.subscribe(std::forward<decltype(f)>(f));
        } else if constexpr (std::is_same_v<event_type, KeyPressedEvent>) {
            m_key_pressed_handler.subscribe(std::forward<decltype(f)>(f));
        } else if constexpr (std::is_same_v<event_type, KeyReleasedEvent>) {
            m_key_released_handler.subscribe(std::forward<decltype(f)>(f));
        } else if constexpr (std::is_same_v<event_type, KeyRepeatedEvent>) {
            m_key_repeated_handler.subscribe(std::forward<decltype(f)>(f));
        }
    }

    void onEvent(const Event& event) const {
        std::visit(Overload{ [this](const WindowResizedEvent& window_resized_event) {
                                m_window_resized_handler.invoke(window_resized_event);
                            },
                             [this](const WindowClosedEvent& window_closed_event) {
                                 m_window_closed_handler.invoke(window_closed_event);
                             },
                             [this](const WindowMaximizedEvent& window_maximized_event) {
                                 m_window_maximized_handler.invoke(window_maximized_event);
                             },
                             [this](const WindowMinimalizedEvent& window_minimalized_event) {
                                 m_window_minimized_handler.invoke(window_minimalized_event);
                             },
                             [this](const MousePositionEvent& mouse_position_event) {
                                 m_mouse_position_event_handler.invoke(mouse_position_event);
                             },
                             [this](const MouseWheelEvent& mouse_wheel_event) {
                                 m_mouse_wheel_event_handler.invoke(mouse_wheel_event);
                             },
                             [this](const MouseButtonPressedEvent& mouse_button_pressed_event) {
                                 m_mouse_button_pressed_handler.invoke(mouse_button_pressed_event);
                             },
                             [this](const MouseButtonReleasedEvent& mouse_button_released_event) {
                                 m_mouse_button_released_event_handler.invoke(mouse_button_released_event);
                             },
                             [this](const KeyPressedEvent& key_pressed_event) {
                                 m_key_pressed_handler.invoke(key_pressed_event);
                             },
                             [this](const KeyReleasedEvent& key_released_event) {
                                 m_key_released_handler.invoke(key_released_event);
                             },
                             [this](const KeyRepeatedEvent& key_repeated_event) {
                                 m_key_repeated_handler.invoke(key_repeated_event);
                             } },
                   event);
    }

private:
    WindowResizedEventHandler m_window_resized_handler{};
    WindowClosedEventHandler m_window_closed_handler{};
    WindowMaximalizedEventHandler m_window_maximized_handler{};
    WindowMinimalizedEventHandler m_window_minimized_handler{};

    MousePositionEventHandler m_mouse_position_event_handler{};
    MouseWheelEventHandler m_mouse_wheel_event_handler{};
    MouseButtonPressedEventHandler m_mouse_button_pressed_handler{};
    MouseButtonReleasedEventHandler m_mouse_button_released_event_handler{};

    KeyPressedEventHandler m_key_pressed_handler{};
    KeyRepeatedEventHandler m_key_repeated_handler{};
    KeyReleasedEventHandler m_key_released_handler{};
};

}// namespace th
