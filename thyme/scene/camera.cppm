export module th.scene.camera;

import std;
import glm;

import th.core.events;
import th.platform.window_event_handler;

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

enum struct CameraProjectionMode {
    perspective,
    orthographic
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

struct PerspectiveCameraArguments {
    float fov;
    float znear;
    float zfar;
    glm::vec2 resolution;
};

struct FpsCameraArguments {
    PerspectiveCameraArguments perspective_camera_arguments;
    glm::vec3 position;
    glm::vec3 direction = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    YawPitchRoll yaw_pitch_roll;
};

/*class Camera {
public:
    virtual ~Camera() = default;

    virtual void setResolution(const glm::vec2& resolution) noexcept = 0;
    virtual void updateViewProjectionMatrix() = 0;
    virtual void updateViewMatrix() = 0;
    virtual void updateProjectionMatrix() = 0;

    [[nodiscard]] virtual auto getProjectionMatrix() const noexcept -> const glm::mat4& = 0;

    [[nodiscard]] virtual auto getViewMatrix() const noexcept -> const glm::mat4& = 0;

    [[nodiscard]] virtual auto getViewProjectionMatrix() const noexcept -> const glm::mat4& = 0;

    [[nodiscard]] virtual auto getPosition() const noexcept -> const glm::vec3 = 0;
    [[nodiscard]] virtual auto getDirection() const noexcept -> const glm::vec3 = 0;

    virtual auto moveForward(float offset) noexcept -> void = 0;
    virtual auto moveLeft(float offset) noexcept -> void = 0;

    virtual auto move(glm::vec2 offset) noexcept -> void = 0;
};*/

class FpsCamera {
    constexpr static glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f);

public:
    explicit FpsCamera(const FpsCameraArguments& camera_arguments);

    FpsCamera() {
        updateVectors();
        updateProjectionMatrix();
    };

    FpsCameraArguments camera_arguments;

    void updateViewProjectionMatrix();
    void updateViewMatrix();
    void updateProjectionMatrix();

    void setResolution(const glm::vec2& resolution) noexcept {
        camera_arguments.perspective_camera_arguments.resolution = resolution;
        updateViewProjectionMatrix();
    }

    void setPosition(const glm::vec3& position) noexcept {
        camera_arguments.position = position;
        updateViewProjectionMatrix();
    }

    void setUp(const glm::vec3& up) noexcept {
        camera_arguments.up = up;
        updateViewProjectionMatrix();
    }

    [[nodiscard]] auto getProjectionMatrix() const noexcept -> const glm::mat4& {
        return m_projection_matrix;
    }

    [[nodiscard]] auto getViewMatrix() const noexcept -> const glm::mat4& {
        return m_view_matrix;
    }

    [[nodiscard]] auto getViewProjectionMatrix() const noexcept -> const glm::mat4& {
        return m_view_projection_matrix;
    }

    [[nodiscard]] auto getPosition() const noexcept -> const glm::vec3 {
        return camera_arguments.direction;
    }
    [[nodiscard]] auto getDirection() const noexcept -> const glm::vec3 {
        return camera_arguments.direction;
    }

    auto move(const glm::vec2 offset) noexcept -> void {
        m_position += m_front * glm::vec3(offset.x);
        m_position += m_right * glm::vec3(offset.y);
        updateViewMatrix();
    }

    auto rotate(const glm::vec2 offset) noexcept -> void {
        m_yaw += offset.x;
        m_pitch += offset.y;
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
        updateVectors();
    }

private:
    void updateVectors() noexcept;

private:
    glm::vec3 m_position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 m_front;
    glm::vec3 m_up = glm::vec3(world_up);
    glm::vec3 m_right;

    float m_yaw{ -90.0f };
    float m_pitch{ 0.0f };

    float m_movement_speed{ 1.0f };
    float m_mouse_sensitivity{ 1.0f };
    float m_zoom{ 0.0f };

    glm::mat4 m_projection_matrix = glm::mat4(1.0f);
    glm::mat4 m_view_matrix = glm::mat4(1.0f);
    glm::mat4 m_view_projection_matrix = glm::mat4(1.0f);
};

[[nodiscard]] auto calculateYawPithRollAngles(glm::vec3 dir, glm::mat3 world_axis) noexcept -> YawPitchRoll;

class DollCamera {
public:
    explicit DollCamera(const CameraArguments& camera_arguments) : camera_arguments{ camera_arguments } {
        updateViewMatrix();
        updateProjectionMatrix();
    }

    void updateProjectionMatrix() {
        m_projection_matrix = glm::gtc::perspective(glm::radians(camera_arguments.fov),
                                                    camera_arguments.resolution.x / camera_arguments.resolution.y,
                                                    camera_arguments.znear,
                                                    camera_arguments.zfar);
        m_projection_matrix[1][1] *= -1.0f;

        m_view_projection_matrix = m_projection_matrix * m_view_matrix;
    }

    void setResolution(const glm::vec2& resolution) {
        camera_arguments.resolution = resolution;
        updateProjectionMatrix();
    }

    void setFov(const float fov) {
        camera_arguments.fov = fov;
        updateProjectionMatrix();
    }

    void setZNear(const float z_near) {
        camera_arguments.znear = z_near;
        updateProjectionMatrix();
    }

    void setZFar(const float z_far) {
        camera_arguments.zfar = z_far;
        updateProjectionMatrix();
    }

    void updateViewMatrix();

    void setCenter(const glm::vec3& center) {
        camera_arguments.center = center;
        updateViewMatrix();
    }

    void setPosition(const glm::vec3& position) {
        camera_arguments.position = position;
        updateViewMatrix();
    }

    void setUp(const glm::vec3& up) {
        camera_arguments.up = up;
        updateViewMatrix();
    }

    void setYawPitchRoll(const YawPitchRoll& yaw_pitch_roll) {
        camera_arguments.yaw_pitch_roll = yaw_pitch_roll;
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
        return camera_arguments.position;
    }

    CameraArguments camera_arguments;

private:
    glm::mat4 m_projection_matrix = glm::mat4(1.0f);
    glm::mat4 m_view_matrix = glm::mat4(1.0f);
    glm::mat4 m_view_projection_matrix = glm::mat4(1.0f);
};

using Camera = std::variant<std::reference_wrapper<FpsCamera>>;

class CameraController {
public:
    explicit CameraController(Camera camera, WindowEventsHandlers& window_events_handler);

    void dispatchEvent(const MousePositionEvent& mouse_position_event);
    void dispatchEvent(const MouseButtonPressedEvent& mouse_button_pressed_event);
    void dispatchEvent(const MouseButtonReleasedEvent& mouse_button_released_event);
    void dispatchEvent(const KeyPressedEvent& key_pressed_event);
    void dispatchEvent(const KeyReleasedEvent& key_released_event);
    void dispatchEvent(const KeyRepeatedEvent& key_repeated_event) const;
    void dispatchEvent(const MouseWheelEvent& mouse_wheel_event);

    void update(float dt);

private:
    Camera m_camera;
    glm::vec2 m_pos{ 0.0f, 0.0f };
    bool m_mouse_left_button_pressed{ false };
    float m_move_speed{ 1.0f };
    float m_rotate_speed{ 5.0f };

    glm::vec2 m_move_offset{ 0.0f, 0.0f };
    glm::vec2 m_rotate_offset{ 0.0f, 0.0f };
};

}// namespace th
