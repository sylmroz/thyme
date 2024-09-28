module;

#include <functional>

export module thyme.core.platform_context;

export namespace Thyme {

struct PlatformContextArguments {
    std::function<void(void)> initializer;
    std::function<void(void)> destroyer;
};

class PlatformContext {
public:
    explicit PlatformContext(const PlatformContextArguments& args) noexcept {
        args.initializer();
        m_destroyer = args.destroyer;
    }

    explicit PlatformContext(const PlatformContext&) = default;
    explicit PlatformContext(PlatformContext&&) = default;

    PlatformContext& operator=(const PlatformContext&) = default;
    PlatformContext& operator=(PlatformContext&&) = default;

    ~PlatformContext() {
        m_destroyer();
    }

private:
    std::function<void(void)> m_destroyer;
};

}// namespace Thyme