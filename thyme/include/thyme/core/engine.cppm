module;

#include <thyme/export_macros.hpp>

#include <string>

export module thyme.core.engine;
import thyme.core.layer;
import thyme.core.layer_stack;

namespace Thyme {

export struct EngineConfig {
    std::string engineName{ "Thyme" };
    std::string appName;
    uint32_t width{ 1920 };
    uint32_t height{ 1080 };
};

export class THYME_API Engine final {
public:
    explicit Engine(const EngineConfig& engineConfig, LayerStack<Layer>& layers);

    void run() const;

private:
    EngineConfig m_engineConfig;
    LayerStack<Layer>& m_layers;
};

}// namespace Thyme