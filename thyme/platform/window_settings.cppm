export module th.platform.window_settings;

import std;
import glm;

import th.platform.window;
import th.core.events;

namespace th {

export class WindowSettings {
public:
    explicit WindowSettings(WindowConfig default_config) : m_config{ std::move(default_config) } {
        load();
    }
    [[nodiscard]] auto getConfig() const -> WindowConfig {
        return m_config;
    }

    void setResolution(const glm::uvec2 resolution) {
        m_config.width = resolution.x;
        m_config.height = resolution.y;
        save();
    }

    void setMaximized(const bool maximized) {
        m_config.maximized = maximized;
        save();
    }

    void operator()(const WindowMaximize& maximize) {
        std::println("maximize: {}", maximize.maximized);
        setMaximized(maximize.maximized);
    }

    void operator()(const WindowResize& resize) {
        setResolution(glm::uvec2(resize.width, resize.height));
    }

private:
    void load();
    void save() const;

    static auto getPathToFile() -> std::filesystem::path;

private:
    WindowConfig m_config;
};


export class WindowSettingsEventDispatcher : public EventDispatcher<WindowSettingsEventDispatcher> {

public:
    explicit WindowSettingsEventDispatcher(WindowSettings& window_settings): m_window_settings{window_settings} {}

    void operator()(const WindowMaximize& maximize) const {
        std::println("maximize: {}", maximize.maximized);
        m_window_settings.setMaximized(maximize.maximized);
    }

    void operator()(const WindowResize& resize) const {
        m_window_settings.setResolution(glm::uvec2(resize.width, resize.height));
    }

    using EventDispatcher::operator();

private:
    WindowSettings& m_window_settings;
};

}// namespace th
