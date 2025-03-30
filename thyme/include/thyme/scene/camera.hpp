#pragma once

#include <thyme/export_macros.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace th::scene {

struct THYME_API CameraArguments {
    float fov;
    float zNear;
    float zFar;
    glm::vec2 resolution;
    glm::vec3 eye;
    glm::vec3 center;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
};

class THYME_API Camera {
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

    void setFov(float fov) {
        m_cameraArguments.fov = fov;
        updateProjectionMatrix();
    }

    void setZNear(float zNear) {
        m_cameraArguments.zNear = zNear;
        updateProjectionMatrix();
    }

    void setZFar(float zFar) {
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

    [[nodiscard]] inline const glm::mat4& getProjectionMatrix() const noexcept {
        return m_projectionMatrix;
    }

    [[nodiscard]] inline const glm::mat4& getViewMatrix() const noexcept {
        return m_viewMatrix;
    }

    [[nodiscard]] inline const glm::mat4& getViewProjectionMatrix() const noexcept {
        return m_viewProjectionMatrix;
    }

private:
    glm::mat4 m_projectionMatrix = glm::mat4(1.0f);
    glm::mat4 m_viewMatrix = glm::mat4(1.0f);
    glm::mat4 m_viewProjectionMatrix = glm::mat4(1.0f);
    CameraArguments m_cameraArguments;
};

}// namespace th::scene