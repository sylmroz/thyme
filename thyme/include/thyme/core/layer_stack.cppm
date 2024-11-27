module;

#include <thyme/export_macros.hpp>

export module thyme.core.layer_stack;

namespace Thyme {

export enum class LayerType : uint8_t {
    overlay,
    non_overlay
};

export template <typename Layer>
class THYME_API LayerStack {
public:
    inline void pushLayer(Layer* layer) {
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

    inline void pushOverlay(Layer* layer) {
        const auto it = std::next(m_layers.begin(), m_nextOverlayIndex);
        m_layers.emplace(it, layer);
        ++m_nextOverlayIndex;
    }

    inline void popOverlay(Layer* layer) {
        m_layers.remove(layer);
        if (m_nextOverlayIndex > 0) {
            --m_nextOverlayIndex;
        }
    }

    inline void pushNonOverlayLayer(Layer* layer) {
        m_layers.push_back(layer);
    }

    inline void popNonOverlayLayer(Layer* layer) {
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

    [[nodiscard]] inline auto end() const noexcept{
        return m_layers.end();
    }

private:
    uint32_t m_nextOverlayIndex{ 0 };
    std::list<Layer*> m_layers;
};

}
