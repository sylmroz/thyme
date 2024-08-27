#pragma once

#include <thyme/export_macros.hpp>

#include "engine.hpp"

#include <string>

namespace Thyme {
class THYME_API Application {
public:
    Application();
    std::string name{"thyme"};
    void run();
private:
    Engine engine;
};
}