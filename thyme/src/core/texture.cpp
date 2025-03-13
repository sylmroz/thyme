#include <thyme/core/texture.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace th {

Texture::Texture(const std::filesystem::path& file) {
    if (!std::filesystem::exists(file) || !std::filesystem::is_regular_file(file)) {
        throw std::invalid_argument(std::format("File {} does not exist", file.string()));
    }
    int texWidth, texHeight, texChannels;
    const auto str = file.string();
    const auto pixels =
            stbi_load(str.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    m_resolution.width = static_cast<uint32_t>(texWidth);
    m_resolution.height = static_cast<uint32_t>(texHeight);
    const auto size = texWidth * texHeight * 4;
    m_data = std::vector(pixels, pixels + size);
    stbi_image_free(pixels);
}

}// namespace Thyme