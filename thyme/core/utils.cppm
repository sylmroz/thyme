module;

#include <algorithm>
#include <string_view>

export module th.core.utils;

namespace th::core {

export template <typename InnerArray, typename OuterArray>
[[nodiscard]] constexpr auto arrayContainsArray(const InnerArray& innerArray, const OuterArray& outerArray)
        -> bool {
    return std::ranges::all_of(innerArray, [&](const auto& innerElement) {
        return std::ranges::any_of(outerArray, [&](const auto& outerElement) {
            return std::string_view(innerElement) == std::string_view(outerElement);
        });
    });
}

}// namespace th::core
