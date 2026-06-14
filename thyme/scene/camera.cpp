module th.scene.camera;

import std;
import th.core.mouse_codes;
import th.core.key_codes;

namespace th {

FpsCamera::FpsCamera(const FpsCameraArguments& camera_arguments) : camera_arguments(camera_arguments) {
    updateViewProjectionMatrix();
}
void FpsCamera::updateViewProjectionMatrix() {
    updateViewMatrix();
    updateProjectionMatrix();
    m_view_projection_matrix = m_projection_matrix * m_view_matrix;
}

void FpsCamera::updateViewMatrix() {
    constexpr auto world_front = glm::vec3(0.0f, 1.0f, 0.0f);
    constexpr auto world_up = glm::vec3(0.0f, 0.0f, 1.0f);
    /*auto direction = glm::rotate(world_front, glm::radians(camera_arguments.yaw_pitch_roll.yaw), world_up);
    const auto right = glm::normalize(glm::cross(direction, world_up));
    direction = glm::normalize(glm::rotate(direction, glm::radians(camera_arguments.yaw_pitch_roll.pitch), right));*/
    //    const auto [yaw, pitch, roll] = camera_arguments.yaw_pitch_roll;
    /*const auto rotation = glm::quat(glm::vec3(glm::radians(pitch),glm::radians(yaw), glm::radians(roll)));
    const auto direction = glm::normalize(rotation * world_front);*/

    m_view_matrix =
            glm::lookAt(camera_arguments.position, camera_arguments.position + camera_arguments.direction, world_up);

    /*const auto pitch_rotation = glm::angleAxis(pitch, glm::vec3 { 1.f, 0.f, 0.f });
    const auto yaw_rotation = glm::angleAxis(yaw, glm::vec3 { 0.f, -1.f, 0.f });
    const auto rotation_matrix = glm::toMat4(yaw_rotation) * glm::toMat4(pitch_rotation);
    const auto translation_matrix = glm::translate(glm::identity<glm::mat4>(), -camera_arguments.position);
    m_view_matrix = glm::inverse(translation_matrix * rotation_matrix);*/
}

void FpsCamera::updateProjectionMatrix() {
    const auto [fov, znear, zfar, resolution] = camera_arguments.perspective_camera_arguments;
    m_projection_matrix = glm::perspective(glm::radians(fov), resolution.x / resolution.y, znear, zfar);
    m_projection_matrix[1][1] *= -1.0f;
}

auto calculateYawPithRollAngles(const glm::vec3 dir, glm::mat3 world_axis) noexcept -> YawPitchRoll {
    const auto calculate_angle =
            [](const glm::vec3 d, const glm::vec3 axis, const glm::vec3 world_axis_project) -> float {
        const auto proj_u = glm::normalize(d - glm::proj(d, world_axis_project));
        const auto cos_alpha = glm::dot(proj_u, axis);
        const auto arc_cos = std::acos(cos_alpha);
        return glm::degrees(arc_cos);
    };

    return YawPitchRoll{
        .yaw = -calculate_angle(dir, glm::vec3(0.0f, 1.0f, 0.0f), world_axis[2]),
        .pitch = calculate_angle(dir, glm::vec3(1.0f, 0.0f, 0.0f), world_axis[1]),
        .roll = calculate_angle(dir, glm::vec3(0.0f, 0.0f, 1.0f), world_axis[0]),
    };
}

void DollCamera::updateViewMatrix() {
    const auto pith_quat =
            glm::quat(glm::radians(camera_arguments.yaw_pitch_roll.pitch) / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    const auto yaw_quat =
            glm::quat(glm::radians(camera_arguments.yaw_pitch_roll.yaw) / 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    const auto roll_quat =
            glm::quat(glm::radians(camera_arguments.yaw_pitch_roll.roll) / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f));

    const auto rot_quat = pith_quat * roll_quat * yaw_quat;
    const auto rot_matrix = glm::toMat4(rot_quat);


    const auto v = glm::lookAt(camera_arguments.position, camera_arguments.center, camera_arguments.up);
    m_view_matrix = rot_matrix * glm::translate(glm::identity<glm::mat4>(), -camera_arguments.position);
    m_view_projection_matrix = m_projection_matrix * m_view_matrix;
}

CameraController::CameraController(Camera camera, WindowEventsHandlers& window_events_handler) : m_camera(camera) {
    window_events_handler.addEventListener<MousePositionEvent>([this](const MousePositionEvent& event) {
        dispatchEvent(event);
    });
    window_events_handler.addEventListener<MouseButtonPressedEvent>([this](const MouseButtonPressedEvent& event) {
        dispatchEvent(event);
    });
    window_events_handler.addEventListener<MouseButtonReleasedEvent>([this](const MouseButtonReleasedEvent& event) {
        dispatchEvent(event);
    });
    window_events_handler.addEventListener<KeyPressedEvent>([this](const KeyPressedEvent& event) {
        dispatchEvent(event);
    });
    window_events_handler.addEventListener<KeyReleasedEvent>([this](const KeyReleasedEvent& event) {
        dispatchEvent(event);
    });
    window_events_handler.addEventListener<MouseWheelEvent>([this](const MouseWheelEvent& event) {
        dispatchEvent(event);
    });
    window_events_handler.addEventListener<KeyRepeatedEvent>([this](const KeyRepeatedEvent& event) {
        dispatchEvent(event);
    });
}

void CameraController::dispatchEvent(const MousePositionEvent& mouse_position_event) {
    const auto distance = m_pos - mouse_position_event.position;
    if (m_mouse_left_button_pressed) {}
    m_pos = mouse_position_event.position;
}

void CameraController::dispatchEvent(const MouseButtonPressedEvent& mouse_button_pressed_event) {
    if (mouse_button_pressed_event.button == MouseButton::button_left) {
        m_mouse_left_button_pressed = true;
    }
}

void CameraController::dispatchEvent(const MouseButtonReleasedEvent& mouse_button_released_event) {
    if (mouse_button_released_event.button == MouseButton::button_left) {
        m_mouse_left_button_pressed = false;
    }
}

void CameraController::dispatchEvent(const KeyPressedEvent& key_pressed_event) {
    switch (key_pressed_event.code) {
        case KeyCode::w: m_move_offset.x += m_speed; break;
        case KeyCode::s: m_move_offset.x -= m_speed; break;
        case KeyCode::a: m_move_offset.y += m_speed; break;
        case KeyCode::d: m_move_offset.y -= m_speed; break;
        default:;
    }
}

void CameraController::dispatchEvent(const KeyRepeatedEvent& key_repeated_event) const {
}

void CameraController::dispatchEvent(const KeyReleasedEvent& key_released_event) {
    switch (key_released_event.code) {
        case KeyCode::w: m_move_offset.x -= m_speed; break;
        case KeyCode::s: m_move_offset.x += m_speed; break;
        case KeyCode::a: m_move_offset.y -= m_speed; break;
        case KeyCode::d: m_move_offset.y += m_speed; break;
        default:;
    }
}

void CameraController::dispatchEvent(const MouseWheelEvent& mouse_wheel_event) {}

void CameraController::update(float dt) {
    std::visit([this, dt](auto camera) {
        camera.get().move(m_move_offset * dt);
    }, m_camera);
}

}// namespace th