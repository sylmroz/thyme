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

private:
    void load();
    void save() const;

    static auto getPathToFile() -> std::filesystem::path;

private:
    WindowConfig m_config;
};

}// namespace th
