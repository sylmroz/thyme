module th.scene.camera;

namespace th {

void Camera::updateViewMatrix() {
    const auto pith_quat =
            glm::quat(glm::radians(m_camera_arguments.yaw_pitch_roll.pitch/2), glm::vec3(1.0f, 0.0f, 0.0f));
    const auto yaw_quat = glm::quat(glm::radians(m_camera_arguments.yaw_pitch_roll.yaw/2), glm::vec3(0.0f, 1.0f, 0.0f));
    const auto roll_quat = glm::quat(glm::radians(m_camera_arguments.yaw_pitch_roll.roll/2), glm::vec3(0.0f, 0.0f, 1.0f));

    const auto rot_quat = pith_quat * roll_quat * yaw_quat;
    const auto rot_matrix = glm::toMat4(rot_quat);

    m_view_matrix = glm::gtc::lookAt(m_camera_arguments.eye, m_camera_arguments.center, m_camera_arguments.up);
    m_view_matrix *= rot_matrix;
    m_view_projection_matrix = m_projection_matrix * m_view_matrix;
}
}// namespace th