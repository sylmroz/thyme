#pragma once

#include <glm/glm.hpp>

namespace th {

struct MVP {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

}// namespace Thyme