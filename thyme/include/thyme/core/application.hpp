#pragma once

#include <thyme/export_macros.hpp>

#include <string>

namespace Thyme {
class THYME_API Application {
public:
    Application();
    std::string name{ "Thyme" };
    void run();
};

}// namespace Thyme