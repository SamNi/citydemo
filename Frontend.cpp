#include "./Frontend.h"
#include "./Backend.h"
// after some thought, I have decided to give the blessing of gl calls to the front end as well
// front end responsibility: upload textures, VBOs, give intermediate geometric forms
// to the backend. cache things maybe.
// idea: Visibility deltas. Both the front end and back end are stateful
// only have relevant changes propagate from frontend to backend.
#include <unordered_set>
#include "essentials.h"
#include "./Frontend.h"
#include "./GL.h"
#include "GLM.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

// bog standard free cam via euler angle
// TODO(SamNi): Look into gimbal lock, quaternions, determine if you
// really need to do something more sophisticated than what we've
// got now
static const glm::vec3 CAMERA_I_AXIS(1.0, 0.0, 0.0);
static const glm::vec3 CAMERA_J_AXIS(0.0, 1.0, 0.0);
static const glm::vec3 CAMERA_K_AXIS(0.0, 0.0, 1.0);

struct Camera {
    explicit Camera(void) : m_modelview_dirty(true),
        m_projection_dirty(true),
        m_field_of_view(glm::radians(60.0f)),
        m_aspect_ratio(16.0f/9.0f),
        m_camera_position(0.0, 0.0, 1.0f),
        m_camera_view(0.0, 0.0, 0.0),
        m_camera_up(CAMERA_J_AXIS),
        yaw(0.0), pitch(0.0), roll(0.0)
    { }
    inline void look_at(const glm::vec3& src, const glm::vec3& dest) {
        m_modelview_dirty = true;
        m_camera_position = src;
    }
    inline void sanitize(glm::vec3& v) {
        if (glm::isnan(v.x) || glm::isinf(v.x))
            v = glm::vec3(0.0f);
    }
    // expects normalized [-1..1], [-1..1] not [0..1920), [0..1080)
    void strafe(const glm::vec3& dir) {
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
    void mouselook(float delta_x, float delta_y) {
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
    void move(const glm::vec3& dir) {
        m_camera_position += dir;
        m_camera_view += dir;
    }
    const glm::mat4x4& get_modelview(void) {
        if (!m_modelview_dirty)
            return m_ret_modelview;
        recompute_modelview();
        return m_ret_modelview;
    }
    const glm::mat4x4& get_projection(void) {
        if (!m_projection_dirty)
            return m_ret_projection;
        recompute_projection();
        return m_ret_projection;
    }
    inline void set_position(const glm::vec3& p) { m_camera_position = p; m_modelview_dirty = true; }
    inline const glm::vec3 get_position(void) const { return m_camera_position; }

    inline void change_fov(float fov) {
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
private:
    inline void recompute_modelview(void) {
        m_ret_modelview = glm::lookAt(m_camera_position, m_camera_view, m_camera_up);
        m_modelview_dirty = false;

        LOG(LOG_TRACE, "Position: %lf %lf %lf", m_camera_position.x, m_camera_position.y, m_camera_position.z);
        LOG(LOG_TRACE, "View: %lf %lf %lf", m_camera_view.x, m_camera_view.y, m_camera_view.z);
    }
    inline void recompute_projection(void) {
        m_ret_projection = glm::perspective(m_field_of_view, m_aspect_ratio, 0.01f, 100.0f);
        m_projection_dirty = false;
    }
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

std::unique_ptr<Frontend::Impl> Frontend::mImpl = nullptr;

struct Frontend::Impl {
    bool startup(int w, int h) {
        if (!Backend::startup(w, h)) {
            LOG(LOG_CRITICAL, "Backend::Startup returned false\n");
            return false;
        }
        
        modes = glfwGetVideoModes(glfwGetPrimaryMonitor(), &nVidmodes);
        for (int i = 0;i < nVidmodes;++i) {
            LOG(LOG_INFORMATION, "%d %d at %dHz (R%dG%dB%d)",
                modes[i].width,
                modes[i].height,
                modes[i].refreshRate,
                modes[i].redBits,
                modes[i].greenBits,
                modes[i].blueBits);
        }


        return true;
    }
    Camera cam;
    void render(void) {
        Backend::set_modelview(cam.get_modelview());
        Backend::set_projection(cam.get_projection());

        Backend::enable_depth_testing();
        Backend::enable_blending();
        Backend::begin_frame();
        Backend::add_tris();
        Backend::end_frame(); // With this call, the backend sets off to do its thing
        Backend::disable_blending();
        Backend::disable_depth_testing();

    }
    void shutdown(void) {
        // free up any of our own resources
        Backend::shutdown();
    }

    const GLFWvidmode *modes;
    int nVidmodes;
};

bool Frontend::startup(int w, int h) {
    mImpl = std::unique_ptr<Frontend::Impl>(new Frontend::Impl());
    return mImpl->startup(w, h);
}
void Frontend::strafe(const glm::vec3& dir) { mImpl->cam.strafe(dir); }
void Frontend::mouselook(float delta_x, float delta_y) { mImpl->cam.mouselook(delta_x, delta_y); }
void Frontend::render(void) { mImpl->render(); }
void Frontend::shutdown(void) {
    mImpl->shutdown();
    mImpl.reset(nullptr);
}

void Frontend::change_fov(float fov) { mImpl->cam.change_fov(fov); }