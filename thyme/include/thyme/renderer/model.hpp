#pragma once

#include <thyme/renderer/structs.hpp>

#include <optional>
#include <vector>

namespace th::renderer {

class Model {
    explicit Model() = default;

public:
    glm::vec4 solidColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    glm::mat4 transformMatrix = glm::mat4(1.0f);
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::optional<Texture> texture;

    void translate(glm::vec3) {}
    void rotate(glm::vec3) {}
    void scale(glm::vec3) {}
};

}// namespace th::renderer
