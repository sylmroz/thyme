#pragma once

#include <cstdint>

namespace th::renderer {

class SwapChain {
public:
    SwapChain(const SwapChain&) = default;
    SwapChain(SwapChain&&) = default;
    SwapChain& operator=(const SwapChain&) = default;
    SwapChain& operator=(SwapChain&&) = default;

    virtual uint32_t prepareFrame() = 0;
    virtual void submitFrame() = 0;

    virtual ~SwapChain() = default;
};

}// namespace th::renderer
