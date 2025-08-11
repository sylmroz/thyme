module;

export module th.platform.platform_context;

namespace th::platform {

/*export template <typename Platform, typename Backend>
requires(std::is_base_of_v<typename Platform::Tag, Backend>)
class PlatformContext: public Platform, public Backend {
    using Platform::Platform;
    using Backend::Backend;
};*/

export template <typename Platform>
class PlatformContext: public Platform, public Platform::Backend {
    using Platform::Platform;
    using Platform::Backend::Backend;
};

}// namespace th::platform
