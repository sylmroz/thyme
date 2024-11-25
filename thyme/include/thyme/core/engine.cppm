module;

#include <thyme/export_macros.hpp>

#include <string>

export module thyme.core.engine;

namespace Thyme {

export struct EngineConfig {
    std::string engineName{ "Thyme" };
    std::string appName;
    uint32_t width{ 1920 };
    uint32_t height{ 1080 };
};

export class THYME_API Engine {
public:
    explicit Engine(const EngineConfig& engineConfig);

    explicit Engine(const Engine&) = default;
    explicit Engine(Engine&&) = default;

    Engine& operator=(const Engine&) = default;
    Engine& operator=(Engine&&) = default;

    void run() const;
    virtual ~Engine() = default;

private:
    EngineConfig m_engineConfig;
};

}// namespace Thyme