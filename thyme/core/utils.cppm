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

export auto readFile(const std::filesystem::path& filePath) -> std::vector<char> {
    const auto fileName = filePath.filename().string();
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + fileName);
    }
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    file.close();
    return buffer;
}


}// namespace th::core
