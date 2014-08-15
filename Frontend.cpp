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
#include "Camera.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

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