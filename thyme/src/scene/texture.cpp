#include <thyme/scene/vulkan_texture.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <fmt/format.h>

namespace th {

TextureData::TextureData(const std::filesystem::path& file) {
    if (!std::filesystem::exists(file) || !std::filesystem::is_regular_file(file)) {
        throw std::invalid_argument(fmt::format("File {} does not exist", file.string()));
    }
    int texWidth{};
    int texHeight{};
    int texChannels{};
    const auto str = file.string();
    auto* const pixels = stbi_load(str.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (pixels == nullptr || texWidth <= 0 || texHeight <= 0) {
        throw std::runtime_error(fmt::format("Failed to load texture {}", str));
    }
    m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    m_resolution.width = static_cast<uint32_t>(texWidth);
    m_resolution.height = static_cast<uint32_t>(texHeight);
    const auto size = texWidth * texHeight * 4;
    m_data = std::vector(pixels, pixels + size);
    stbi_image_free(pixels);
}

}// namespace th