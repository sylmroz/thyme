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

export auto readFile(const std::filesystem::path& file_path) -> std::vector<char> {
    const auto file_name = file_path.filename().string();
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + file_name);
    }
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    file.close();
    return buffer;
}


}// namespace th
