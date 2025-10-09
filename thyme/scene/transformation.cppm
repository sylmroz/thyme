export module th.scene.transformation;

import glm;

export namespace th {

class Transformation {
public:
    inline void translate(const glm::vec3 offset) noexcept {
        m_center_point += glm::vec4(offset, 0.0f);
        m_transform_matrix = glm::gtc::translate(m_transform_matrix, offset);
    }

    inline void rotate(const float angle, const glm::vec3 axis) noexcept {
        //glm::rotate(glm::mat4(1.0f), deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        m_transform_matrix = glm::gtc::rotate(glm::mat4(1.0f), angle, axis);
    }

    inline void scale(const glm::vec3 factor) noexcept {
        m_transform_matrix = glm::gtc::scale(m_transform_matrix, factor);
    }

    [[nodiscard]] inline auto getTransformMatrix() const noexcept -> glm::mat4 {
        return m_transform_matrix;
    }

    [[nodiscard]] inline auto getCenterPoint() const noexcept -> glm::vec4 {
        return m_center_point;
    }

private:
    glm::vec4 m_center_point = glm::vec4(0.0f);
    glm::mat4 m_transform_matrix = glm::mat4(1.0f);
};

}
