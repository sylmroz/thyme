module;

#include "thyme/export_macros.hpp"

#include <string>

export module thyme.core.application;

namespace Thyme {

export class THYME_API Application {
public:
    Application();
    std::string name{ "Thyme" };
    void run();
};

}// namespace Thyme