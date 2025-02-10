#pragma once

#include <filesystem>
#include <vector>

namespace Thyme {

[[nodiscard]] std::vector<char> readFile(const std::filesystem::path& filePath);

class NoCopyable {
public:
    NoCopyable() = default;
    NoCopyable(const NoCopyable&) = delete;
    NoCopyable& operator=(const NoCopyable&) = delete;
    NoCopyable(NoCopyable&&) = delete;
    NoCopyable& operator=(NoCopyable&&) = delete;

    virtual ~NoCopyable() = default;
};

template <typename... Ts>
struct Overload: Ts... {
    using Ts::operator()...;
};

template <typename... Ts>
Overload(Ts...) -> Overload<Ts...>;

}// namespace Thyme
