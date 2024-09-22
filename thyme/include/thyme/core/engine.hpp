#pragma once

#include <thyme/core/platform_context.hpp>
#include <thyme/export_macros.hpp>

namespace Thyme {

struct EngineConfig {
    std::string engineName{ "Thyme" };
    std::string appName;
};

class THYME_API Engine {
public:
    explicit Engine(EngineConfig engineConfig);

    explicit Engine(const Engine&) = default;
    explicit Engine(Engine&&) = default;

    Engine& operator=(const Engine&) = default;
    Engine& operator=(Engine&&) = default;

    void run();
    virtual ~Engine() = default;

private:
    EngineConfig m_engineConfig;
};

}// namespace Thyme