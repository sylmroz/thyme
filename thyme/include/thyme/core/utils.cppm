module;

#include "logger.hpp"

#include <filesystem>
#include <fstream>

export module thyme.core.utils;

export namespace Thyme {

std::vector<char> readFile(const std::filesystem::path& filePath) {
    const auto fileName = filePath.filename().string();
    TH_API_LOG_INFO("Reading from file {}", fileName);
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        TH_API_LOG_ERROR("Could not open file {}", fileName);
        throw std::runtime_error("Could not open file " + fileName);
    }
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    file.close();
    return buffer;
}

class NoCopyable {
public:
    NoCopyable() = default;
    NoCopyable(const NoCopyable&) = delete;
    NoCopyable& operator=(const NoCopyable&) = delete;
    NoCopyable(NoCopyable&&) = delete;
    NoCopyable& operator=(NoCopyable&&) = delete;

    virtual ~NoCopyable() = default;
};

template<typename... Ts>
struct Overload: Ts... {
    using Ts::operator()...;
};

template<typename... Ts>
Overload(Ts...) -> Overload<Ts...>;

}// namespace Thyme
