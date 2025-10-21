module;

#include <cstdint>

module th.platform.window_settings;

import std;
import nlohmann.json;

struct WindowConfigFile {
    uint32_t width;
    uint32_t height;
    bool maximized;
};

static void to_json(nlohmann::json& j, const WindowConfigFile& cfg) {
    j = nlohmann::json{ { "width", cfg.width }, { "height", cfg.height }, { "maximized", cfg.maximized } };
}

static void from_json(const nlohmann::json& j, WindowConfigFile& cfg) {
    j.at("width").get_to(cfg.width);
    j.at("height").get_to(cfg.height);
    j.at("maximized").get_to(cfg.maximized);
}

namespace th {

void WindowSettings::load() {
    const auto path_to_file = getPathToFile();
    if (!std::filesystem::exists(path_to_file)) {
        return;
    }
    std::ifstream file(path_to_file);
    const auto [width, height, maximized] = nlohmann::json::parse(file).get<WindowConfigFile>();
    m_config.width = width;
    m_config.height = height;
    m_config.maximized = maximized;
}

void WindowSettings::save() const {
    const auto path_to_file = getPathToFile();
    std::ofstream file{ path_to_file };
    const auto cfg_string = nlohmann::json(WindowConfigFile{ .width = m_config.width,
                                                             .height = m_config.height,
                                                             .maximized = m_config.maximized })
                                    .dump();
    file.write(cfg_string.data(), cfg_string.size());
}

auto WindowSettings::getPathToFile() -> std::filesystem::path {
#ifndef NDEBUG
    return "./window_config.json";
#else
    return "./window_config.json";// TODO
#endif
}

}// namespace th