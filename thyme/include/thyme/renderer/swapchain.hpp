#pragma once

#include <cstdint>

namespace th::renderer {

class SwapChain {
public:
    SwapChain() = default;
    SwapChain(const SwapChain&) = default;
    SwapChain(SwapChain&&) = default;
    SwapChain& operator=(const SwapChain&) = default;
    SwapChain& operator=(SwapChain&&) = default;

    virtual bool prepareFrame() = 0;
    virtual void submitFrame() = 0;

    virtual ~SwapChain() = default;
};

}// namespace th::renderer
