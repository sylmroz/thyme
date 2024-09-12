#pragma once

#include <thyme/export_macros.hpp>

namespace Thyme {

struct EngineConfig {
    std::string engineName{ "Thyme" };
    std::string appName;
};

class THYME_API Engine {
public:
    Engine(const EngineConfig& engineConfig);
    void run();

private:
    EngineConfig m_engineConfig;
};

}// namespace Thyme