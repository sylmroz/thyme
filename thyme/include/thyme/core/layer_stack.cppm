module;

#include <thyme/export_macros.hpp>

export module thyme.core.layer_stack;

import thyme.core.layer;

export namespace Thyme {

class THYME_API LayerStack {
public:
    void pushLayer(Layer* layer) {
        if (layer->getType() == LayerType::overlay) {
            pushOverlay(layer);
        } else {
            pushNonOverlayLayer(layer);
        }
    }

    inline void popLayer(Layer* layer) {
        if (layer->getType() == LayerType::overlay) {
            popOverlay(layer);
        } else {
            popNonOverlayLayer(layer);
        }
    }

    [[nodiscard]] inline auto begin() noexcept {
        return m_layers.begin();
    }

    [[nodiscard]] inline auto begin() const noexcept {
        return m_layers.begin();
    }

    [[nodiscard]] inline auto end() noexcept {
        return m_layers.end();
    }

    [[nodiscard]] inline auto end() const noexcept{
        return m_layers.end();
    }

private:
    inline void pushOverlay(Layer* layer) {
        const auto it = std::next(m_layers.begin(), m_lastOverlayIndex);
        m_layers.emplace(it, layer);
        ++m_lastOverlayIndex;
    }

    inline void popOverlay(Layer* layer) {
        m_layers.remove(layer);
        if (m_lastOverlayIndex > 0) {
            --m_lastOverlayIndex;
        }
    }

    inline void pushNonOverlayLayer(Layer* layer) {
        m_layers.push_back(layer);
    }

    inline void popNonOverlayLayer(Layer* layer) {
        m_layers.remove(layer);
    }

private:
    uint32_t m_lastOverlayIndex{ 0 };
    std::list<Layer*> m_layers;
};

}
