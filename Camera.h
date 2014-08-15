#ifndef _CAMERA_H
#define _CAMERA_H

#include "essentials.h"
#include "GLM.h"

// bog standard free cam via euler angle
// TODO(SamNi): Look into gimbal lock, quaternions, determine if you
// really need to do something more sophisticated than what we've
// got now

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
    struct Impl;
    Impl *m_impl;
    // TODO(SamNi): I really would much prefer to use unique_ptr
    // for this, but couldn't get it to work without making the
    // implementation member static, which we do not want for
    // the camera class.
};

#endif  // ~CAMERA_H