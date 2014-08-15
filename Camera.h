#ifndef _CAMERA_H
#define _CAMERA_H

#include "essentials.h"
#include "GLM.h"

// bog standard free cam via euler angle
// TODO(SamNi): Look into gimbal lock, quaternions, determine if you
// really need to do something more sophisticated than what we've
// got now
static const glm::vec3 CAMERA_I_AXIS(1.0, 0.0, 0.0);
static const glm::vec3 CAMERA_J_AXIS(0.0, 1.0, 0.0);
static const glm::vec3 CAMERA_K_AXIS(0.0, 0.0, 1.0);

struct Camera {
    explicit Camera(void);
    void look_at(const glm::vec3& src, const glm::vec3& dest);
    void sanitize(glm::vec3& v);

    // expects normalized [-1..1], [-1..1] not [0..1920), [0..1080)
    void strafe(const glm::vec3& dir);
    void mouselook(float delta_x, float delta_y);
    void move(const glm::vec3& dir);
    const glm::mat4x4& get_modelview(void);
    const glm::mat4x4& get_projection(void);
    void set_position(const glm::vec3& p);
    const glm::vec3& get_position(void) const;
    void change_fov(float fov);

private:
    void recompute_modelview(void);
    void recompute_projection(void);

    glm::vec3 m_camera_position;        // where is the camera located? 
    glm::vec3 m_camera_view;            // where is it pointing
    glm::vec3 m_camera_up;              // opposite direction of gravity
    double yaw, pitch, roll;

    // this values is derived from the above, caching
    glm::vec3 m_camera_lateral;

    Real m_field_of_view;       // in radians
    Real m_aspect_ratio;

    glm::mat4x4 m_ret_modelview;
    glm::mat4x4 m_ret_projection;
    bool m_modelview_dirty;     // caching the computed matrices
    bool m_projection_dirty;
};

#endif  // ~CAMERA_H