#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

namespace th::scene {

class Transformation {
public:
    inline void translate(const glm::vec3 offset) noexcept {
        centerPoint += glm::vec4(offset, 0.0f);
        transformMatrix = glm::translate(transformMatrix, offset);
    }

    inline void rotate(const float angle, const glm::vec3 axis) noexcept {
        transformMatrix = glm::rotate(transformMatrix, angle, axis);
    }

    inline void scale(const glm::vec3 factor) noexcept {
        transformMatrix = glm::scale(transformMatrix, factor);
    }

    [[nodiscard]] inline glm::mat4 getTransformMatrix() const noexcept {
        return transformMatrix;
    }
    
    [[nodiscard]] inline glm::vec4 getCenterPoint() const noexcept {
        return centerPoint;
    }

private:
    glm::vec4 centerPoint = glm::vec4(0.0f);
    glm::mat4 transformMatrix = glm::mat4(1.0f);
};

}// namespace th::scene