export module th.core.utils;

import std;

namespace th {

export template <typename InnerArray, typename OuterArray>
[[nodiscard]] constexpr auto arrayContainsArray(const InnerArray& inner_array, const OuterArray& outer_array) -> bool {
    return std::ranges::all_of(inner_array, [&](const auto& inner_element) {
        return std::ranges::any_of(outer_array, [&](const auto& outer_element) {
            return std::string_view(inner_element) == std::string_view(outer_element);
        });
    });
}

export
template <typename Container>
auto readFile(const std::filesystem::path& file_path) -> Container {
    const auto file_name = file_path.filename().string();
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + file_name);
    }
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    Container buffer;
    buffer.resize(size);
    file.read(buffer.data(), size);
    file.close();
    return buffer;
}

export template<typename T, typename ... U>
concept either = (std::same_as<T, U> || ...);


}// namespace th
