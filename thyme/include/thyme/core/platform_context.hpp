#pragma once

#include <functional>

namespace Thyme {

struct PlatformContextArgumenst {
    std::function<void(void)> initializer;
    std::function<void(void)> destroyer;
};

class PlatformContext {
public:
    explicit PlatformContext(const PlatformContextArgumenst& args) noexcept {
        args.initializer();
        m_destroyer = args.destroyer;
    }

    PlatformContext(PlatformContext&) = default;
    PlatformContext(PlatformContext&&) = default;

    PlatformContext& operator=(PlatformContext&) = default;
    PlatformContext& operator=(PlatformContext&&) = default;

    ~PlatformContext() {
        m_destroyer();
    }

private:
    std::function<void(void)> m_destroyer;
};

}// namespace Thyme