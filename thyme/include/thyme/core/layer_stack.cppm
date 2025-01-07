module;

#include <thyme/export_macros.hpp>

export module thyme.core.layer_stack;

import thyme.core.layer;

export namespace Thyme {

template <typename... Args>
class THYME_API LayerStack {
public:
    inline void pushLayer(Layer<Args...>* layer) {
        if (layer->getType() == LayerType::overlay) {
            pushOverlay(layer);
        } else {
            pushNonOverlayLayer(layer);
        }
    }

    inline void popLayer(Layer<Args...>* layer) {
        if (layer->getType() == LayerType::overlay) {
            popOverlay(layer);
        } else {
            popNonOverlayLayer(layer);
        }
    }

    inline void pushOverlay(Layer<Args...>* layer) {
        const auto it = std::next(m_layers.begin(), m_nextOverlayIndex);
        m_layers.emplace(it, layer);
        ++m_nextOverlayIndex;
    }

    inline void popOverlay(Layer<Args...>* layer) {
        m_layers.remove(layer);
        if (m_nextOverlayIndex > 0) {
            --m_nextOverlayIndex;
        }
    }

    inline void pushNonOverlayLayer(Layer<Args...>* layer) {
        m_layers.push_back(layer);
    }

    inline void popNonOverlayLayer(Layer<Args...>* layer) {
        m_layers.remove(layer);
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

    [[nodiscard]] inline auto end() const noexcept {
        return m_layers.end();
    }

private:
    uint32_t m_nextOverlayIndex{ 0 };
    std::list<Layer<Args...>*> m_layers;
};

}// namespace Thyme
