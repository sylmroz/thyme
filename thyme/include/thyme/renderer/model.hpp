#pragma once

#include <thyme/renderer/structs.hpp>

#include <vector>

namespace th::renderer {

class Model {
public:
    glm::vec4 solidColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    glm::mat4 transformMatrix = glm::mat4(1.0f);
    glm::vec4 originPoint;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    Texture texture; // TODO! make array of textures

    void translate(glm::vec3 offset) {}
    void rotate(float angle, glm::vec3 axis) {}
    void scale(glm::vec3 factor) {}
};

}// namespace th::renderer
