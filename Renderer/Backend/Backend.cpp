// Back end dependencies: TextureManager, ShaderManager, ResourceManager
// Copyright [year] <Copyright Owner>
// Goal: isolate the direct OpenGL calls, enums to the back end
#include "Backend_local.h"
#include "../../LuaBindings.h"
#include "GL.h"
#include "Shader.h"
#include "../../Camera.h"
#include <png.h>            // for writing screenshots
#include "ResourceManager.h"

#pragma warning(disable : 4800)

static const int        OFFSCREEN_WIDTH =           512;
static const int        OFFSCREEN_HEIGHT =          512;
static const bool       PIXELATED =                 false;

#include "../../GUI.h"

using namespace GUI;

struct MyWidget : public Widget {
    explicit MyWidget(void) { }
    virtual void draw(void) const {
        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, 15);
    }
};

struct MyWidgetManager : public WidgetManager {
    explicit MyWidgetManager(void) {
    }
protected:
    struct WidgetUBO {
        static const uint32_t MAX_NUM_MVP = 1024;
        static const uint32_t MVP_SIZE = sizeof(glm::mat4x4);
        static const uint32_t BUF_SIZE = MAX_NUM_MVP*MVP_SIZE;

        explicit WidgetUBO(void) {
            m_ubo = ImmutableBufPtr(new OpenGLBufferImmutable(GL_UNIFORM_BUFFER, GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT, BUF_SIZE));
            m_ubo->bind();

            // Assign said UBO to uniform
            GLint prog_handle;
            glGetIntegerv(GL_CURRENT_PROGRAM, &prog_handle);

            auto block_index = glGetUniformBlockIndex(prog_handle, "per_instance_mvp");
            if (block_index == GL_INVALID_INDEX) {
                LOG(LOG_CRITICAL, "block_index == GL_INVALID_INDEX");
                return;
            }
            glUniformBlockBinding(prog_handle, block_index, 0);
            glBindBufferBase(GL_UNIFORM_BUFFER, block_index, m_ubo->get_handle());

            for (uint16_t i = 0;i < MAX_NUM_MVP;++i) {
                glm::mat4x4 mvp(1.0f);
                mvp *= glm::translate(glm::vec3(uniform(-1.0f, 1.0f), uniform(-1.0f, 1.0f), 0.0f));
                mvp *= glm::scale(glm::vec3(uniform(0.05f, 0.35f)));
                mvp *= glm::rotate((float)glm::radians(uniform(-180.0f,180.0f)), glm::vec3(0.0f, 0.0f, 1.0f));
                mvp *= glm::ortho(-1.77777f, 1.77777f, -1.0f, 1.0f);
                set_mvp(i, mvp);
            }
            glBindBuffer(GL_UNIFORM_BUFFER, 0);

            LOG(LOG_TRACE, "WidgetUBO opened");
            LOG(LOG_TRACE, "Allocated space for %u 4x4 MVPs (%u kB)", MAX_NUM_MVP, BUF_SIZE >> 10);
        }
        void set_mvp(uint16_t index, const glm::mat4x4& mat) {
            assert(index < MAX_NUM_MVP);
            glBindBuffer(GL_UNIFORM_BUFFER, m_ubo->get_handle());
            auto p = glMapBufferRange(GL_UNIFORM_BUFFER, index*MVP_SIZE, MVP_SIZE, GL_MAP_WRITE_BIT);
            memcpy(p, glm::value_ptr(mat), MVP_SIZE);
            glUnmapBuffer(GL_UNIFORM_BUFFER);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }

    protected:
        ImmutableBufPtr m_ubo;
    };

protected:
    WidgetUBO m_widget_ubo;
};

static Framebuffer *offscreenFB = nullptr;

static const int NUM_TRIANGLES = 1000;
SurfaceTriangles st(3*NUM_TRIANGLES, 3*NUM_TRIANGLES);

struct Backend::Impl {
    inline void clear_performance_counters(void) { wipe_memory(&m_perf_count, sizeof(PerfCounters)); }
    inline void query_hardware_specs(void) {
        glGetIntegerv(GL_NUM_EXTENSIONS, &mSpecs.nExtensions);
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &mSpecs.nMaxCombinedTextureImageUnits);
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &mSpecs.nMaxDrawBuffers);
        glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &mSpecs.nMaxElementsIndices);
        glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &mSpecs.nMaxElementsVertices);
        glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_BLOCKS, &mSpecs.nMaxGeometryUniformBlocks);
        glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &mSpecs.nMaxFragmentUniformBlocks);
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &mSpecs.nMaxTextureImageUnits);
        glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS , &mSpecs.nMaxUniformBufferBindings);
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &mSpecs.nMaxVertexAttribs);
        glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &mSpecs.nMaxVertexUniformBlocks);

        mSpecs.renderer = glGetString(GL_RENDERER);
        mSpecs.vendor = glGetString(GL_VENDOR);
        mSpecs.version = glGetString(GL_VERSION);
        LOG(LOG_INFORMATION, "%s", mSpecs.renderer);
        LOG(LOG_INFORMATION, "%s", mSpecs.vendor);
        LOG(LOG_INFORMATION, "%s", mSpecs.version);
        mSpecs.extensions.resize(mSpecs.nExtensions);

        for (int i = 0;i < mSpecs.nExtensions;++i) {
            static const unsigned char *c;
            c = glGetStringi(GL_EXTENSIONS, i);
            LOG(LOG_TRACE, "extension: %s", c);
            mSpecs.extensions[i] = c;
        }

    }
    GeometryBuffer geom_buf;

    bool startup(int w, int h) {
        if (false == Lua::startup())
            return false;

        if (false == m_resource_manager.startup())
            return false;

        // misc. defaults
        current_screen_width = w;
        current_screen_height = h;
        m_draw_hud = true;
        offscreenRender = PIXELATED;

        clear_performance_counters();
        query_hardware_specs();

        return true;
    }
    void shutdown(void) {
        QuadVAO::reset();
        TextureManager::shutdown();
        m_resource_manager.shutdown();
        Lua::shutdown();
    }

    void begin_frame(void) {
        clear_performance_counters();
        if (offscreenRender) {
            if (nullptr == offscreenFB)
                offscreenFB = new Framebuffer(OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
            offscreenFB->bind();    
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        geom_buf.cmd_queues.Clear();
    }

    uint32_t loc;

    void add_random_tris(void) {
        static bool firstTime = true;
        if (false == firstTime)
            return;
        firstTime = false;
        GLuint i;
        for (i = 0;i < 3*NUM_TRIANGLES;i += 3) {
            glm::vec3 v0(uniform(-1.5f, 1.5f), uniform(-1.5f, 1.5f), uniform(-1.5f, 1.5f));
            glm::vec3 v1(v0 + glm::vec3(uniform(-0.25f, 0.25f), uniform(-0.25f, 0.25f), uniform(-0.25f, 0.25f)));
            glm::vec3 v2(v0 + glm::vec3(uniform(-0.25f, 0.25f), uniform(-0.25f, 0.25f), uniform(-0.25f, 0.25f)));
            st.vertices[i+0] = v0;
            st.vertices[i+1] = v1;
            st.vertices[i+2] = v2;
            st.texture_coordinates[i+0] = TexCoord(0, 0);
            st.texture_coordinates[i+1] = TexCoord(65535, 0);
            st.texture_coordinates[i+2] = TexCoord(0, 65535);
            st.normals[i+0] = normal_pack(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
            st.normals[i+1] = normal_pack(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
            st.normals[i+2] = normal_pack(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
            st.indices[i+0] = i+0;
            st.indices[i+1] = i+1;
            st.indices[i+2] = i+2;
        }
        mImpl->geom_buf.open_geometry_buffer();
        loc = mImpl->geom_buf.add_surface(st);
        mImpl->geom_buf.close_geometry_buffer();

    }
    uint32_t add_surface_triangles(std::shared_ptr<SurfaceTriangles> st) {
        geom_buf.open_geometry_buffer();
        auto ret = geom_buf.add_surface(*st);
        geom_buf.close_geometry_buffer();
        m_surface_triangles.emplace(ret, st);
        return ret;
    }
    void draw_surface_triangles(uint32_t handle) {
        auto it = m_surface_triangles.find(handle);
        if (it == m_surface_triangles.end()) {
            LOG(LOG_WARNING, "draw_surface_triangles called on missing handle %u", handle);
            return;
        }
        geom_buf.open_command_queue();
        geom_buf.add_draw_command(it->first, it->second->nIndices);
        geom_buf.close_command_queue();
    }
    void end_frame(void) {
        set_instanced_mode(false);
        glBindVertexArray(geom_buf.get_vao());
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(geom_buf.cmd_queues.get_base_offset_in_bytes()), geom_buf.cmd_queues.get_size(), 0);
        if (true == m_draw_hud) {
            disable_depth_testing();

            // Draw 2D GUI elements
            static WidgetPtr my_widget(new MyWidget());
            static MyWidgetManager widget_manager;
            static bool firstTime = true;

            if (firstTime) {
                widget_manager.set_root(my_widget);
                widget_manager.set_focus(my_widget);
            }
            glBindVertexArray(QuadVAO::get_vao());
            set_instanced_mode(true);
            widget_manager.draw();

            firstTime = false;
            set_instanced_mode(false);
        }
        if (offscreenRender)
            offscreenFB->blit(current_screen_width, current_screen_height);
        geom_buf.cmd_queues.Swap();
    }
    void resize(int w, int h) {
        glViewport(0, 0, w, h);
        current_screen_width = w;
        current_screen_height = h;
    }
    RGBPixel* get_screenshot(void) const {
        uint32_t buf_size = current_screen_width*current_screen_height*sizeof(RGBPixel);
        RGBPixel *ret = new RGBPixel[buf_size];
        glReadPixels(0, 0, current_screen_width, current_screen_height, GL_RGB, GL_UNSIGNED_BYTE, ret);
        imgflip(current_screen_width, current_screen_height, 3, (uint8_t*)ret);
        return ret;
    }
    RGBPixel* read_screenshot(const char *fname) const {
	    png_image image;
        uint8_t* ret = nullptr;

	    memset(&image, 0, sizeof(image));
	    if (!png_image_begin_read_from_file(&image, fname))
		    return nullptr;
	    ret = new uint8_t[PNG_IMAGE_SIZE(image)];

	    if (ret == nullptr)
		    return nullptr;
	    if (!png_image_finish_read(&image, NULL, ret, 0, NULL))
            return nullptr;
	    return (RGBPixel*)ret;
    }

    void write_screenshot(void) {
        static char date_time_string[128] = "";
        static char filename[1024] = "";
        static time_t raw_time;

        raw_time = time(nullptr);
        strftime(date_time_string, 127, "%Y%m%d_%H%M%S", localtime(&raw_time));
        sprintf(filename, "screenshot_%s_%03u.png", date_time_string, ++screenshotCounter);
        write_screenshot(filename);
    }
    bool write_screenshot(const char *filename) {
        int nBytes = 0;
        png_image image = { NULL };

        LOG(LOG_INFORMATION, "Screenshot %dx%d to %s", current_screen_width, current_screen_height, filename);

        nBytes = current_screen_width*current_screen_height*3*sizeof(uint8_t);
        auto buf = get_screenshot();

        image.width = current_screen_width;
        image.height = current_screen_height;
        image.version = PNG_IMAGE_VERSION;
        image.format = PNG_FORMAT_RGB;

        if (!png_image_write_to_file(&image, filename, 0, (void*)buf, 0, nullptr)) {
            LOG(LOG_WARNING, "Failed to write screenshot to %s", filename);
            delete[] buf;
            return false;
        }
        delete[] buf;
        return true;
    }

    void disable_blending(void) { glDisable(GL_BLEND); }
    void show_hud(bool b) { m_draw_hud = b; }

    void enable_additive_blending(void) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }
    void enable_blending(void) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void set_clear_color(const RGBPixel& c) const { glClearColor(c.r/255.0f, c.g/255.0f, c.b/255.0f, 1.0f); }
    void set_clear_color(const RGBAPixel& c) const { glClearColor(c.r/255.0f, c.g/255.0f, c.b/255.0f, c.a/1.0f); }
    void enable_depth_testing(void) { glEnable(GL_DEPTH_TEST); }
    void disable_depth_testing(void) { glDisable(GL_DEPTH_TEST); }

    void set_modelview(const glm::mat4x4& m) {
        GLint program_handle;
        glGetIntegerv(GL_CURRENT_PROGRAM, &program_handle);
        glUniformMatrix4fv(glGetUniformLocation(program_handle, "modelView"), 1, GL_FALSE, glm::value_ptr(m));
    }
    void set_projection(const glm::mat4x4& m) {
        GLint program_handle;
        glGetIntegerv(GL_CURRENT_PROGRAM, &program_handle);
        glUniformMatrix4fv(glGetUniformLocation(program_handle, "projection"), 1, GL_FALSE, glm::value_ptr(m));
    }
    void set_instanced_mode(bool b) {
        GLint program_handle;
        glGetIntegerv(GL_CURRENT_PROGRAM, &program_handle);
        auto loc = glGetUniformLocation(program_handle, "gui_quad_instanced");

        glUniform1ui(loc, b);
    }
    int current_screen_width;
    int current_screen_height;
    bool m_draw_hud;
    ResourceManager m_resource_manager;

    std::map<uint32_t, std::shared_ptr<SurfaceTriangles>> m_surface_triangles;

    bool offscreenRender;

    PerfCounters m_perf_count;
    const PerfCounters& get_performance_count(void) const { return m_perf_count; }

    Specs mSpecs;

    // declaring this statically so the counter is preserved
    // in between backend power cycles and the automated
    // tests don't write over each other's results
    static uint16_t screenshotCounter;
};
uint16_t Backend::Impl::screenshotCounter = 0;

std::unique_ptr<Backend::Impl> Backend::mImpl = nullptr;

// public interface
bool Backend::startup(int w, int h) {
    mImpl = std::unique_ptr<Impl>(new Impl());
    return mImpl->startup(w, h);
}

void Backend::shutdown(void) {
    mImpl->shutdown();
    mImpl.reset(nullptr);
}

void Backend::begin_frame(void) { mImpl->begin_frame(); }
void Backend::end_frame(void) { mImpl->end_frame(); }
void Backend::resize(int w, int h) { mImpl->resize(w, h); }
RGBPixel* Backend::get_screenshot(void) { return mImpl->get_screenshot(); }
RGBPixel* Backend::read_screenshot(const char *path) { return mImpl->read_screenshot(path); }
void Backend::write_screenshot(void) { mImpl->write_screenshot(); }
bool Backend::write_screenshot(const char *filename) { return mImpl->write_screenshot(filename); }
void Backend::add_random_tris(void) { mImpl->add_random_tris(); }
uint32_t Backend::add_surface_triangles(std::shared_ptr<SurfaceTriangles> st) { return mImpl->add_surface_triangles(st); }
void Backend::draw_surface_triangles(uint32_t handle) { mImpl->draw_surface_triangles(handle); }
void Backend::set_modelview(const glm::mat4x4& m) { mImpl->set_modelview(m); }
void Backend::set_projection(const glm::mat4x4& m) { mImpl->set_projection(m); }
void Backend::set_clear_color(const RGBPixel& c) { mImpl->set_clear_color(c); }
void Backend::set_clear_color(const RGBAPixel& c) { mImpl->set_clear_color(c); }
void Backend::enable_depth_testing(void) { mImpl->enable_depth_testing(); }
void Backend::disable_depth_testing(void) { mImpl->disable_depth_testing(); }
void Backend::enable_blending(void) { mImpl->enable_blending(); }
void Backend::enable_additive_blending(void) { mImpl->enable_additive_blending(); }
void Backend::disable_blending(void) { mImpl->disable_blending(); }
void Backend::show_hud(bool b) { mImpl->show_hud(b); }
const PerfCounters& Backend::get_performance_count(void) { return mImpl->get_performance_count(); }