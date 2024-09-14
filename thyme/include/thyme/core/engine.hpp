#pragma once

#include <thyme/export_macros.hpp>
#include <thyme/core/platform_context.hpp>

namespace Thyme {

struct EngineConfig {
    std::string engineName{ "Thyme" };
    std::string appName;
};

class THYME_API Engine {
public:
    Engine(const EngineConfig& engineConfig, const PlatformContext& context);
    void run();

private:
    EngineConfig m_engineConfig;
    const PlatformContext& m_context;
};

}// namespace Thyme