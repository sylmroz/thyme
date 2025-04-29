#pragma once

#include <thyme/core/common_structs.hpp>

#include <filesystem>
#include <mdspan>
#include <vector>

namespace th {

class TextureData {
public:
    explicit TextureData(const std::filesystem::path& file);
    TextureData(const uint32_t mipLevel, const Resolution resolution, const std::vector<uint8_t>& data)
        : m_mipLevels{ mipLevel }, m_resolution{ resolution }, m_data{ data } {}
    [[nodiscard]] auto getMipLevels() const noexcept -> uint32_t {
        return m_mipLevels;
    }

    [[nodiscard]] auto getResolution() const noexcept -> Resolution {
        return m_resolution;
    }

    [[nodiscard]] auto getData() const noexcept -> std::span<uint8_t const> {
        return m_data;
    }

private:
    uint32_t m_mipLevels;
    Resolution m_resolution{};
    std::vector<uint8_t> m_data;
};


}// namespace Thyme