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

std::unique_ptr<Frontend::Impl> Frontend::mImpl = nullptr;

struct Frontend::Impl {
    bool startup(int w, int h) {
        if (!Backend::startup(w, h)) {
            LOG(LOG_CRITICAL, "Backend::Startup returned false\n");
            return false;
        }
        Backend::set_clear_color(RGBPixel(51, 51, 51));
        modes = glfwGetVideoModes(glfwGetPrimaryMonitor(), &m_num_videomodes);
        for (int i = 0;i < m_num_videomodes;++i) {
            LOG(LOG_INFORMATION, "%d %d at %dHz (R%dG%dB%d)",
                modes[i].width,
                modes[i].height,
                modes[i].refreshRate,
                modes[i].redBits,
                modes[i].greenBits,
                modes[i].blueBits);
        }
        auto st = std::shared_ptr<SurfaceTriangles>(new SurfaceTriangles(3, 3));
        st->vertices[0] = glm::vec3(-1.0f, -1.0f, 0.0f);
        st->vertices[1] = glm::vec3(+1.0f, -1.0f, 0.0f);
        st->vertices[2] = glm::vec3(+1.0f, +1.0f, 0.0f);
        st->indices[0] = 0;
        st->indices[1] = 1;
        st->indices[2] = 2;
        st->colors[0] = COLOR_ALPHA[WHITE];
        st->colors[1] = COLOR_ALPHA[WHITE];
        st->colors[2] = COLOR_ALPHA[WHITE];
        st->texture_coordinates[0] = TexCoord(0, 0);
        st->texture_coordinates[1] = TexCoord(65535, 0);
        st->texture_coordinates[2] = TexCoord(65535, 65535);
        st->normals[0] = normal_pack(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
        st->normals[1] = normal_pack(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
        st->normals[2] = normal_pack(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

        st_handle = Backend::add_surface_triangles(st);

        return true;
    }
    uint32_t st_handle;
    Camera cam;
    void render(void) {
        Backend::set_modelview(cam.get_modelview());
        Backend::set_projection(cam.get_projection());
        Backend::set_clear_color(COLOR[BLACK]);
        Backend::show_hud(false);
        Backend::enable_depth_testing();
        Backend::enable_blending();
        Backend::begin_frame();
        Backend::draw_surface_triangles(st_handle);
        //Backend::add_random_tris();
        Backend::end_frame(); // With this call, the backend sets off to do its thing
        //Backend::disable_blending();
        //Backend::disable_depth_testing();

    }
    void shutdown(void) {
        // free up any of our own resources
        Backend::shutdown();
    }

    const GLFWvidmode *modes;
    int m_num_videomodes;
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