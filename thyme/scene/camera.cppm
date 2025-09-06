module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module th.scene.camera;

export namespace th {

struct CameraMatrices {
    glm::mat4 view;
    glm::mat4 projection;
};

struct CameraArguments {
    float fov;
    float zNear;
    float zFar;
    glm::vec2 resolution;
    glm::vec3 eye;
    glm::vec3 center;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
};

class Camera {
public:
    explicit Camera(const CameraArguments& cameraArguments) : m_cameraArguments{ cameraArguments } {
        m_projectionMatrix = glm::perspective(glm::radians(m_cameraArguments.fov),
                                              m_cameraArguments.resolution.x / m_cameraArguments.resolution.y,
                                              m_cameraArguments.zNear,
                                              m_cameraArguments.zFar);
        m_projectionMatrix[1][1] *= -1.0f;
        m_viewMatrix = glm::lookAt(m_cameraArguments.eye, m_cameraArguments.center, m_cameraArguments.up);
        m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
    }

    void updateProjectionMatrix() {
        m_projectionMatrix = glm::perspective(glm::radians(m_cameraArguments.fov),
                                              m_cameraArguments.resolution.x / m_cameraArguments.resolution.y,
                                              m_cameraArguments.zNear,
                                              m_cameraArguments.zFar);
        m_projectionMatrix[1][1] *= -1.0f;
        m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
    }

    void setResolution(const glm::vec2& resolution) {
        m_cameraArguments.resolution = resolution;
        updateProjectionMatrix();
    }

    void setFov(const float fov) {
        m_cameraArguments.fov = fov;
        updateProjectionMatrix();
    }

    void setZNear(const float zNear) {
        m_cameraArguments.zNear = zNear;
        updateProjectionMatrix();
    }

    void setZFar(const float zFar) {
        m_cameraArguments.zFar = zFar;
        updateProjectionMatrix();
    }

    void updateViewMatrix() {
        m_viewMatrix = glm::lookAt(m_cameraArguments.eye, m_cameraArguments.center, m_cameraArguments.up);
        m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
    }
    void setCenter(const glm::vec3& center) {
        m_cameraArguments.center = center;
        updateViewMatrix();
    }

    void setEyePosition(const glm::vec3& eye) {
        m_cameraArguments.eye = eye;
        updateViewMatrix();
    }

    void setUp(const glm::vec3& up) {
        m_cameraArguments.up = up;
        updateViewMatrix();
    }

    [[nodiscard]] inline auto getProjectionMatrix() const noexcept -> const glm::mat4& {
        return m_projectionMatrix;
    }

    [[nodiscard]] inline auto getViewMatrix() const noexcept -> const glm::mat4& {
        return m_viewMatrix;
    }

    [[nodiscard]] inline auto getViewProjectionMatrix() const noexcept -> const glm::mat4& {
        return m_viewProjectionMatrix;
    }

private:
    glm::mat4 m_projectionMatrix = glm::mat4(1.0f);
    glm::mat4 m_viewMatrix = glm::mat4(1.0f);
    glm::mat4 m_viewProjectionMatrix = glm::mat4(1.0f);
    CameraArguments m_cameraArguments;
};
}
