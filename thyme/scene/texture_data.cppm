export module th.scene.texture_data;

import std;

import glm;
import stb.image;

export namespace th {

class TextureData {
public:
    explicit TextureData(const std::filesystem::path& file);
    TextureData(const uint32_t mip_level, const glm::ivec2 resolution, const std::vector<uint8_t>& data)
        : m_mip_levels{ mip_level }, m_resolution{ resolution }, m_data{ data } {}
    [[nodiscard]] auto getMipLevels() const noexcept -> uint32_t {
        return m_mip_levels;
    }

    [[nodiscard]] auto getResolution() const noexcept -> glm::uvec2 {
        return m_resolution;
    }

    [[nodiscard]] auto getData() const noexcept -> std::span<uint8_t const> {
        return m_data;
    }

private:
    uint32_t m_mip_levels;
    glm::uvec2 m_resolution{};
    std::vector<uint8_t> m_data;
};

TextureData::TextureData(const std::filesystem::path& file) {
    if (!std::filesystem::exists(file) || !std::filesystem::is_regular_file(file)) {
        throw std::invalid_argument(std::format("File {} does not exist", file.string()));
    }
    int tex_width{};
    int tex_height{};
    int tex_channels{};
    const auto str = file.string();
    auto* const pixels = stbi_load(str.c_str(), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
    if (pixels == nullptr || tex_width <= 0 || tex_height <= 0) {
        throw std::runtime_error(std::format("Failed to load texture {}", str));
    }
    m_mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(tex_width, tex_height)))) + 1;
    m_resolution.x = static_cast<uint32_t>(tex_width);
    m_resolution.y = static_cast<uint32_t>(tex_height);
    const auto size = tex_width * tex_height * 4;
    m_data = std::vector(pixels, pixels + size);
    stbi_image_free(pixels);
}

}// namespace th
