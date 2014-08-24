#include "./Frontend.h"
#include "../Backend/Backend.h"
// idea: Visibility deltas. Both the front end and back end are stateful
// only have relevant changes propagate from frontend to backend.
#include <unordered_set>
#include "../../essentials.h"
#include "Camera.h"
#include <glfw/glfw3.h>

std::unique_ptr<Frontend::Impl> Frontend::mImpl = nullptr;

struct Frontend::Impl {
    bool startup(int w, int h) {
        if (!Backend::startup(w, h)) {
            LOG(LOG_CRITICAL, "Backend::Startup returned false\n");
            return false;
        }
        if (!TextureManager::startup()) {
            LOG(LOG_CRITICAL, "TextureManager::Startup returned false\n");
            return false;
        }
        Backend::set_clear_color(COLOR[BLACK]);
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
        const int NUM_TRIANGLES = 2500;
        auto st = std::shared_ptr<SurfaceTriangles>(new SurfaceTriangles(3*NUM_TRIANGLES, 3*NUM_TRIANGLES));
        for (auto i = 0;i < 3*NUM_TRIANGLES;++i) {
            st->vertices[i] = glm::vec3(uniform(-1.0f, 1.0f), uniform(-1.0f, 1.0f), uniform(-0.1f, 0.1f));
            st->indices[i] = i;
            st->colors[i] = COLOR_ALPHA[i%NUM_COLORS];
            st->texture_coordinates[i] = TexCoord((uint16_t)(uniform(0, 65535.0f)),uint16_t(uniform(0, 65535.0f)));
            st->normals[i] = normal_pack(glm::vec4(0.0f, 0.0f,-1.0f + uniform(-0.2f, 0.2f), 0.0f));
        }
        st_handle = Backend::add_surface_triangles(st);
        Backend::enable_depth_testing();
        Backend::enable_blending();
        Backend::show_hud(false);
        return true;
    }
    uint32_t st_handle;
    Camera cam;
    void render(void) {
        Backend::set_modelview(cam.get_modelview());
        Backend::set_projection(cam.get_projection());
        Backend::begin_frame();
        Backend::draw_surface_triangles(st_handle);
        Backend::end_frame(); // With this call, the backend sets off to do its thing

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

void Frontend::write_screenshot(void) { Backend::write_screenshot(); }
void Frontend::change_fov(float fov) { mImpl->cam.change_fov(fov); }