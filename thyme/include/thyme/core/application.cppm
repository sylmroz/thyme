module;

#include <thyme/export_macros.hpp>

#include <string>

export module thyme.core.application;

import thyme.core.layer;
import thyme.core.layer_stack;

namespace Thyme {

export class THYME_API Application {
public:
    Application();
    std::string name{ "Thyme" };
    void run();

    template <typename L>
    L addLayer() noexcept {
        return L(m_layers);
    }

private:
    LayerStack<Layer> m_layers;
};

}// namespace Thyme