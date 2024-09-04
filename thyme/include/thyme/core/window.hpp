#pragma once

#include <string>

namespace Thyme {

struct WindowConfiguration {
    uint32_t width{ 0 };
    uint32_t height{ 0 };
    std::string name;
};

class Window {
public:
    Window(const WindowConfiguration& windowConfiguretion) : config{ windowConfiguretion } {}
    virtual void poolEvents() = 0;
    virtual bool shouldClose() = 0;

    virtual ~Window() = default;

    WindowConfiguration config;
};

}// namespace Thyme