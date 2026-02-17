export module th.scene.camera;

import glm;

export namespace th {

struct CameraMatrices {
    glm::mat4 view;
    glm::mat4 projection;
};

struct YawPitchRoll {
    float yaw;
    float pitch;
    float roll;
};

struct CameraArguments {
    float fov;
    float znear;
    float zfar;
    glm::vec2 resolution;
    glm::vec3 position;
    glm::vec3 center;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    YawPitchRoll yaw_pitch_roll;
};

class Camera {
public:
    explicit Camera(const CameraArguments& camera_arguments) : m_camera_arguments{ camera_arguments } {
        updateViewMatrix();
        updateProjectionMatrix();
    }

    void updateProjectionMatrix() {
        m_projection_matrix = glm::gtc::perspective(glm::radians(m_camera_arguments.fov),
                                              m_camera_arguments.resolution.x / m_camera_arguments.resolution.y,
                                              m_camera_arguments.znear,
                                              m_camera_arguments.zfar);
        m_projection_matrix[1][1] *= -1.0f;
        m_view_projection_matrix = m_projection_matrix * m_view_matrix;
    }

    void setResolution(const glm::vec2& resolution) {
        m_camera_arguments.resolution = resolution;
        updateProjectionMatrix();
    }

    void setFov(const float fov) {
        m_camera_arguments.fov = fov;
        updateProjectionMatrix();
    }

    void setZNear(const float z_near) {
        m_camera_arguments.znear = z_near;
        updateProjectionMatrix();
    }

    void setZFar(const float z_far) {
        m_camera_arguments.zfar = z_far;
        updateProjectionMatrix();
    }

    void updateViewMatrix();

    void setCenter(const glm::vec3& center) {
        m_camera_arguments.center = center;
        updateViewMatrix();
    }

    void setPosition(const glm::vec3& position) {
        m_camera_arguments.position = position;
        updateViewMatrix();
    }

    void setUp(const glm::vec3& up) {
        m_camera_arguments.up = up;
        updateViewMatrix();
    }

    void setYawPitchRoll(const YawPitchRoll& yaw_pitch_roll) {
        m_camera_arguments.yaw_pitch_roll = yaw_pitch_roll;
        updateViewMatrix();
    }

    [[nodiscard]] inline auto getProjectionMatrix() const noexcept -> const glm::mat4& {
        return m_projection_matrix;
    }

    [[nodiscard]] inline auto getViewMatrix() const noexcept -> const glm::mat4& {
        return m_view_matrix;
    }

    [[nodiscard]] inline auto getViewProjectionMatrix() const noexcept -> const glm::mat4& {
        return m_view_projection_matrix;
    }

    [[nodiscard]] inline auto getPosition() const noexcept -> const glm::vec3& {
        return m_camera_arguments.position;
    }

private:
    glm::mat4 m_projection_matrix = glm::mat4(1.0f);
    glm::mat4 m_view_matrix = glm::mat4(1.0f);
    glm::mat4 m_view_projection_matrix = glm::mat4(1.0f);
    CameraArguments m_camera_arguments;
};

class CameraController {
    public:
    explicit CameraController(const CameraArguments& camera_arguments);

    void move(const glm::vec3& direction);

private:

};

}
