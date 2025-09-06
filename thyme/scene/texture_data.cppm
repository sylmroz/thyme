module;

#include <glm/vec2.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

export module th.scene.texture_data;

import std;

export namespace th {

class TextureData {
public:
    explicit TextureData(const std::filesystem::path& file);
    TextureData(const uint32_t mipLevel, const glm::ivec2 resolution, const std::vector<uint8_t>& data)
        : m_mipLevels{ mipLevel }, m_resolution{ resolution }, m_data{ data } {}
    [[nodiscard]] auto getMipLevels() const noexcept -> uint32_t {
        return m_mipLevels;
    }

    [[nodiscard]] auto getResolution() const noexcept -> glm::uvec2 {
        return m_resolution;
    }

    [[nodiscard]] auto getData() const noexcept -> std::span<uint8_t const> {
        return m_data;
    }

private:
    uint32_t m_mipLevels;
    glm::uvec2 m_resolution{};
    std::vector<uint8_t> m_data;
};

TextureData::TextureData(const std::filesystem::path& file) {
    if (!std::filesystem::exists(file) || !std::filesystem::is_regular_file(file)) {
        throw std::invalid_argument(std::format("File {} does not exist", file.string()));
    }
    int texWidth{};
    int texHeight{};
    int texChannels{};
    const auto str = file.string();
    auto* const pixels = stbi_load(str.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (pixels == nullptr || texWidth <= 0 || texHeight <= 0) {
        throw std::runtime_error(std::format("Failed to load texture {}", str));
    }
    m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    m_resolution.x = static_cast<uint32_t>(texWidth);
    m_resolution.y = static_cast<uint32_t>(texHeight);
    const auto size = texWidth * texHeight * 4;
    m_data = std::vector(pixels, pixels + size);
    stbi_image_free(pixels);
}

}// namespace th
