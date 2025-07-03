#pragma once

#include <glm/glm.hpp>

namespace th::renderer {

struct MVP {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
};

struct CameraMatrices {
    glm::mat4 view;
    glm::mat4 projection;
};

}// namespace th::renderer