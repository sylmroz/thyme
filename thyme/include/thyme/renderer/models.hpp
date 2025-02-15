#pragma once

#include <glm/glm.hpp>

namespace Thyme {

struct MVP {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

}// namespace Thyme