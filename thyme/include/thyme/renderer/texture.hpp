#pragma once

#include <variant>

#include <thyme/scene/texture_data.hpp>

#include <thyme/platform/vulkan/vulkan_texture.hpp>

namespace th {

using TextureType = std::variant<vulkan::Vulkan2DTexture>;

class Texture2D: public TextureType {
public:
    void setData(const TextureData& textureData) {
        std::visit([&textureData](auto&& underlyingType) { underlyingType.setData(textureData); }, *this);
    }
};

}// namespace th
