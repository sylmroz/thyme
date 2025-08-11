module;

export module th.render_system.swap_chain;

namespace th::render_system {

export class SwapChain {
public:
    SwapChain() = default;
    SwapChain(const SwapChain&) = default;
    SwapChain(SwapChain&&) = default;
    auto operator=(const SwapChain&) -> SwapChain& = default;
    auto operator=(SwapChain&&) -> SwapChain& = default;

    virtual auto prepareFrame() -> bool = 0;
    virtual void submitFrame() = 0;

    virtual ~SwapChain() = default;
};

}// namespace th::render_system
