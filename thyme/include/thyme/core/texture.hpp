#pragma once

#include <thyme/core/common_structs.hpp>

#include <filesystem>
#include <vector>

namespace Thyme {

class Texture {
public:
    explicit Texture(const std::filesystem::path& file);
    Texture(const int mipLevel, const Resolution resolution, const std::vector<uint8_t>& data)
        : m_mipLevels{ mipLevel }, m_resolution{ resolution }, m_data{ data } {}
    [[nodiscard]] auto getMipLevels() const noexcept -> int {
        return m_mipLevels;
    }

    [[nodiscard]] auto getResolution() const noexcept -> Resolution {
        return m_resolution;
    }

    [[nodiscard]] auto getData() const noexcept -> std::span<uint8_t const> {
        return m_data;
    }

private:
    int m_mipLevels;
    Resolution m_resolution;
    std::vector<uint8_t> m_data;
};


}// namespace Thyme