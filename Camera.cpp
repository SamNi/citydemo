#include "Camera.h"

Camera::Camera(void) : m_modelview_dirty(true),
    m_projection_dirty(true),
    m_field_of_view(glm::radians(60.0f)),
    m_aspect_ratio(16.0f/9.0f),
    m_camera_position(0.0, 0.0, 1.0f),
    m_camera_view(0.0, 0.0, 0.0),
    m_camera_up(CAMERA_J_AXIS),
    yaw(0.0), pitch(0.0), roll(0.0)
{ }

void Camera::look_at(const glm::vec3& src, const glm::vec3& dest) {
    m_modelview_dirty = true;
    m_camera_position = src;
}

void Camera::sanitize(glm::vec3& v) {
    if (glm::isnan(v.x) || glm::isinf(v.x))
        v = glm::vec3(0.0f);
}

void Camera::strafe(const glm::vec3& dir) {
    auto lateral = glm::normalize(glm::cross(m_camera_view - m_camera_position, m_camera_up));
    auto vertical = glm::normalize(m_camera_up);
    auto forward = glm::normalize(m_camera_view - m_camera_position);
    sanitize(lateral);
    sanitize(vertical);
    sanitize(forward);

    auto d = dir.x*lateral + dir.y*vertical + dir.z*forward;

    m_camera_position += d;
    m_camera_view += d;

    m_modelview_dirty = true;
}

void Camera::mouselook(float delta_x, float delta_y) {
    auto old_direction = m_camera_view - m_camera_position;
    auto rot_pitch = glm::angleAxis(delta_y, glm::cross(old_direction, m_camera_up));
    auto rot_yaw = glm::angleAxis(-delta_x, m_camera_up);
    auto tmp = glm::normalize(rot_yaw * rot_pitch);

    auto new_direction = glm::rotate(tmp, old_direction);
    auto new_up = glm::rotate(tmp, m_camera_up);

    m_camera_up = new_up;
    m_camera_view = m_camera_position + new_direction;

    m_modelview_dirty = true;
}

void Camera::move(const glm::vec3& dir) {
    m_camera_position += dir;
    m_camera_view += dir;
}

const glm::mat4x4& Camera::get_modelview(void) {
    if (!m_modelview_dirty)
        return m_ret_modelview;
    recompute_modelview();
    return m_ret_modelview;
}

const glm::mat4x4& Camera::get_projection(void) {
    if (!m_projection_dirty)
        return m_ret_projection;
    recompute_projection();
    return m_ret_projection;
}

void Camera::set_position(const glm::vec3& p) {
    m_camera_position = p;
    m_modelview_dirty = true; 
}

const glm::vec3& Camera::get_position(void) const {
    return m_camera_position;
}

void Camera::change_fov(float fov) {
    static const float CHANGE_FOV_SENSITIVITY = glm::pi<float>()/32.0f;
    static const float MIN_FOV = glm::radians(0.5f), MAX_FOV = glm::radians(179.5f);
    m_field_of_view -= CHANGE_FOV_SENSITIVITY*fov;

    if (m_field_of_view < MIN_FOV)
        m_field_of_view = MIN_FOV;
    else if (m_field_of_view > MAX_FOV)
        m_field_of_view = MAX_FOV;

    m_projection_dirty = true;
    LOG(LOG_TRACE, "Fov is now %f degrees", glm::degrees(m_field_of_view));
}

void Camera::recompute_modelview(void) {
    m_ret_modelview = glm::lookAt(m_camera_position, m_camera_view, m_camera_up);
    m_modelview_dirty = false;

    LOG(LOG_TRACE, "Position: %lf %lf %lf", m_camera_position.x, m_camera_position.y, m_camera_position.z);
    LOG(LOG_TRACE, "View: %lf %lf %lf", m_camera_view.x, m_camera_view.y, m_camera_view.z);
}

void Camera::recompute_projection(void) {
    m_ret_projection = glm::perspective(m_field_of_view, m_aspect_ratio, 0.01f, 100.0f);
    m_projection_dirty = false;
}