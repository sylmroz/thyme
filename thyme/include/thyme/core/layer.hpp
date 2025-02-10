#pragma once

#include <thyme/export_macros.hpp>

#include <thyme/core/event.hpp>

#include <stdint.h>

namespace Thyme {

enum class LayerType : uint8_t {
    overlay,
    non_overlay
};

template <typename... Args>
class THYME_API Layer {
public:
    explicit Layer(const LayerType type, const std::string_view name, const bool visible = true)
        : m_type{ type }, m_visible{ visible }, m_name{ name } {}

    explicit Layer(const Layer&) = default;
    explicit Layer(Layer&&) noexcept = default;

    Layer& operator=(const Layer&) = default;
    Layer& operator=(Layer&&) = default;

    virtual void onEvent(const Event& event) = 0;
    virtual void onAttach() = 0;
    virtual void onDetach() = 0;
    virtual void draw(Args&&...) = 0;

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

    virtual ~Layer() noexcept = default;

private:
    LayerType m_type;
    bool m_visible;
    std::string m_name;
};

template <typename... Args>
class THYME_API OverlayLayer: public Layer<Args...> {
public:
    explicit OverlayLayer(const std::string_view name, const bool visible = true)
        : Layer<Args...>{ LayerType::overlay, name, visible } {}
};

template <typename... Args>
class THYME_API NonOverlayLayer: public Layer<Args...> {
public:
    explicit NonOverlayLayer(const std::string_view name, const bool visible = true)
        : Layer<Args...>{ LayerType::non_overlay, name, visible } {}
};

}// namespace Thyme
