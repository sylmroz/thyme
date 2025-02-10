#include <thyme/core/utils.hpp>
#include <thyme/core/logger.hpp>

#include <filesystem>
#include <fstream>


std::vector<char> Thyme::readFile(const std::filesystem::path& filePath) {
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