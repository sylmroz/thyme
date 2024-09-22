#pragma once

#include "thyme/export_macros.hpp"

#include <string>
#include <utility>

namespace Thyme {

struct THYME_API WindowConfiguration {
    uint32_t width{ 0 };
    uint32_t height{ 0 };
    std::string name;
};

class THYME_API Window {
public:
    explicit Window(WindowConfiguration windowConfiguration) : config{std::move( windowConfiguration )} {}

    explicit Window(const Window& window) = default;
    explicit Window(Window&& window) = default;

    Window& operator=(const Window& window) = default;
    Window& operator=(Window&& window) = default;

    virtual void poolEvents() = 0;
    [[nodiscard]] virtual bool shouldClose() = 0;

    WindowConfiguration config;

    virtual ~Window() = default;
};

}// namespace Thyme