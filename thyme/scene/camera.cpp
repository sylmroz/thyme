module th.scene.camera;

import std;

namespace th {

FpsCamera::FpsCamera(const FpsCameraArguments& camera_arguments): camera_arguments(camera_arguments) {
    updateViewProjectionMatrix();
}
void FpsCamera::updateViewProjectionMatrix() {
    updateViewMatrix();
    updateProjectionMatrix();
    m_view_projection_matrix = m_projection_matrix * m_view_matrix;
}

void FpsCamera::updateViewMatrix() {
    /*const auto pith_quat =
            glm::quat(glm::radians(camera_arguments.yaw_pitch_roll.pitch) / 2.0f, camera_arguments.up);
    const auto right = glm::cross(camera_arguments.direction, camera_arguments.up);
    const auto yaw_quat =
            glm::quat(glm::radians(camera_arguments.yaw_pitch_roll.yaw) / 2.0f, right);
    camera_arguments.direction = (pith_quat * yaw_quat) * camera_arguments.direction;*/
    m_view_matrix = glm::lookAt(camera_arguments.position, camera_arguments.position + camera_arguments.direction, camera_arguments.up);
}

void FpsCamera::updateProjectionMatrix() {
    const auto [fov, znear, zfar, resolution] = camera_arguments.perspective_camera_arguments;
    m_projection_matrix = glm::perspective(fov, resolution.x / resolution.y, znear, zfar);
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
}// namespace th