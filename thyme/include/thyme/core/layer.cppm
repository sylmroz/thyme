module;

#include <thyme/export_macros.hpp>

export module thyme.core.layer;

import thyme.core.event;
import thyme.core.layer_stack;

export namespace Thyme {

class THYME_API Layer {
public:
    explicit Layer(const LayerType type, const std::string_view name, LayerStack<Layer>& layers,
                   const bool visible = true)
        : m_type{ type }, m_visible{ visible }, m_name{ name }, m_layers{ layers } {
        layers.pushLayer(this);
    }

    explicit Layer(const Layer&) = delete;
    explicit Layer(Layer&& layer) noexcept = delete;

    Layer& operator=(const Layer&) = delete;
    Layer& operator=(Layer&&) = delete;

    virtual void onEvent(const Event& event) = 0;
    virtual void onAttach() = 0;
    virtual void onDetach() = 0;
    virtual void draw() = 0;

    [[nodiscard]] inline std::string_view getName() const noexcept {
        return m_name;
    }
    [[nodiscard]] inline LayerType getType() const noexcept {
        return m_type;
    }
    [[nodiscard]] inline bool isVisible() const noexcept {
        return m_visible;
    }

    inline void show() noexcept {
        m_visible = true;
    }

    inline void hide() noexcept {
        m_visible = false;
    }

    virtual ~Layer() noexcept {
        m_layers.popLayer(this);
    };

private:
    LayerType m_type;
    bool m_visible;
    std::string m_name;
    LayerStack<Layer>& m_layers;
};

class THYME_API OverlayLayer: public Layer {
public:
    explicit OverlayLayer(const std::string_view name, LayerStack<Layer>& layers, const bool visible = true)
        : Layer{ LayerType::overlay, name, layers, visible } {}
};

class THYME_API NonOverlayLayer: public Layer {
public:
    explicit NonOverlayLayer(const std::string_view name, LayerStack<Layer>& layers, const bool visible = true)
        : Layer{ LayerType::non_overlay, name, layers, visible } {}
};

}// namespace Thyme
