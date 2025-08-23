module;

export module th.platform.platform_context;

export namespace th {

template <typename Platform, typename Backend>
class PlatformContext: public Platform, public Backend {
    using Platform::Platform;
    using Backend::Backend;
};

template <typename Platform>
using VulkanBackendPlatformContext = PlatformContext<Platform, typename Platform::VulkanBackend>;

using HeadlessPlatformContext = PlatformContext<void, void>;

}// namespace th
