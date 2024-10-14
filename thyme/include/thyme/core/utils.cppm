module;

#include "logger.hpp"


#include <fstream>
#include <filesystem>

export module thyme.core.utils;

export namespace Thyme {

std::vector<char> readFile(const std::filesystem::path& filePath) {
    const auto fileName = filePath.filename().string();
    TH_API_LOG_INFO("Reading from file {}", fileName);
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        TH_API_LOG_ERROR("Could not open file {}", fileName);
        throw std::runtime_error("Could not open file " + fileName);
    }
    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    file.close();
    return buffer;
}

}// namespace Thyme
