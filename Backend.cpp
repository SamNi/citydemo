// Copyright [year] <Copyright Owner>
// Goal: isolate the direct OpenGL calls, enums to the back end
// Singletons are ugly but I can't think of anything better for this
#include "./Backend.h"
#include "./GL.h"
#include "./GLM.h"
#include "Shader.h"
#include <png.h>            // for writing screenshots
#include <GLFW/glfw3.h>     // glfwGetTime()

#pragma warning(disable : 4800)

namespace Backend {

static const int        OFFSCREEN_WIDTH =           128;
static const int        OFFSCREEN_HEIGHT =          128;
static const int        TARGET_FPS =                60;

struct Pimpl;
std::unique_ptr<Pimpl> pBackend = nullptr;

struct Pimpl {
    inline void ClearPerformanceCounters(void) { memset(&mPerfCounts, NULL, sizeof(PerfCounters)); }
    inline void QueryHardwareSpecs(void) {
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &mSpecs.nMaxCombinedTextureImageUnits);
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &mSpecs.nMaxDrawBuffers);
        glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &mSpecs.nMaxElementsIndices);
        glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &mSpecs.nMaxElementsVertices);
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &mSpecs.nMaxTextureImageUnits);
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &mSpecs.nMaxVertexAttribs);
    }
    bool Startup(int w, int h) {
        current_screen_width = w;
        current_screen_height = h;
        screenCounter = 0;

        ClearPerformanceCounters();
        QueryHardwareSpecs();

        // Allocate things, prepare VBOs,*/
        offscreenRender = false;
        return true;
    }
    void Shutdown(void) {
        // glDelete* calls should go here
    }
    void BeginFrame(void) {
        ClearPerformanceCounters();
        mPerfCounts.t_initial = glfwGetTime();

        if (offscreenRender) {
            static bool firstTime = true;
            if (firstTime) {
                glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_texID);
                checkGL();
                glGenTextures(1, &fbTexID);
                glBindTexture(GL_TEXTURE_2D, fbTexID);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                checkGL();

                glGenFramebuffers(1, &fbo_id);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fbTexID, 0);
                checkGL();

                firstTime = false;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);  // rendering to offscreen buffer 
            glBindTexture(GL_TEXTURE_2D, old_texID);    // with the image we read
            glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    

        // ... scene draws go in between the conclusion of BeginFrame()
        // and the prologue of EndFrame()
        //TheShaderManager.use("ortho2d");
        DrawFullscreenQuad();
    }
    void EndFrame(void) {
        static double elapsed;

        if (offscreenRender) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);       // switch to the visible render buffer
            glDisable(GL_DEPTH_TEST);
            glClear(GL_DEPTH_BUFFER_BIT);
            glBindTexture(GL_TEXTURE_2D, fbTexID);
            glViewport(0, 0, current_screen_width, current_screen_height);
            Pimpl::DrawFullscreenQuad();
        }
        mPerfCounts.t_final = glfwGetTime();
        
        auto& t0 = mPerfCounts.t_initial;
        auto& t1 = mPerfCounts.t_final;
        double elapsed_ms = 1000*(t1-t0);
        if (elapsed_ms > (1000.0/TARGET_FPS)) {
            // TODO(SamNi): Intentionally taxing the GPU apparently 
            // doesn't trigger this for some reason? Investigate.
            static double past_ms;
            past_ms = elapsed - (1000.0/TARGET_FPS);
            LOG(LOG_TRACE, "frame behind schedule (%lfms elapsed, target is %lf)", elapsed_ms, past_ms);
        }
    }
    void Resize(int w, int h) {
        glViewport(0, 0, w, h);
        current_screen_width = w;
        current_screen_height = h;
    }
    void Screenshot(void) {
        uint8_t *buf = nullptr;
        int nBytes = 0;
        png_image image = { NULL };
        static char filename[512] = { '\0' };

        sprintf(filename, "screenshot%05u.png", screenCounter++);

        LOG(LOG_INFORMATION, "Screenshot %dx%d to %s", current_screen_width, current_screen_height, filename);

        nBytes = current_screen_width*current_screen_height*4*sizeof(uint8_t);
        buf = new uint8_t[nBytes];

        glReadPixels(0, 0, current_screen_width, current_screen_height, GL_RGBA, GL_UNSIGNED_BYTE, buf);
        imgflip(current_screen_width, current_screen_height, 4, buf);

        image.width = current_screen_width;
        image.height = current_screen_height;
        image.version = PNG_IMAGE_VERSION;
        image.format = PNG_FORMAT_RGBA;

        if (!png_image_write_to_file(&image, filename, 0, (void*)buf, 0, nullptr))
            LOG(LOG_WARNING, "Failed to write screenshot to %s", filename);

        delete[] buf;
    }

    void DisableBlending(void) { glDisable(GL_BLEND); }
    void DrawFullscreenQuad(void) {
        static bool firstTime = true;
        // these are all in UpLeft, DownLeft, DownRight, UpRight order
        static const GLfloat points[] = {
            // ccw order starting from lower left
            -1.0f, -1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f,
        };
        static const GLfloat colors[] = {
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
        };
        static const GLfloat texCoords[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f,
        };
        static const GLfloat normals[] = {
            -1.0f, +1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            +1.0f, -1.0f, -1.0f,
            +1.0f, +1.0f, -1.0f,
        };
        static GLuint vbo_position, vbo_colors, vbo_texCoords, vbo_normal, vao;
        if (firstTime) {
            glGenBuffers(1, &vbo_position);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
            glBufferData(GL_ARRAY_BUFFER, 3*4*sizeof(GLfloat), points, GL_STATIC_DRAW);
            checkGL();

            glGenBuffers(1, &vbo_colors);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
            glBufferData(GL_ARRAY_BUFFER, 4*4*sizeof(GLfloat), colors, GL_STATIC_DRAW);
            checkGL();

            glGenBuffers(1, &vbo_texCoords);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_texCoords);
            glBufferData(GL_ARRAY_BUFFER, 2*4*sizeof(GLfloat), texCoords, GL_STATIC_DRAW);
            checkGL();

            glGenBuffers(1, &vbo_normal);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_normal);
            glBufferData(GL_ARRAY_BUFFER, 3*4*sizeof(GLfloat), normals, GL_STATIC_DRAW);
            checkGL();

            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_texCoords);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_normal);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
            checkGL();
            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
            glEnableVertexAttribArray(3);
            checkGL();

            firstTime = false;
        } else
            glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    void EnableAdditiveBlending(void) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }
    void EnableBlending(void) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    int current_screen_width;
    int current_screen_height;

    GLuint fbo_id, fbTexID;
    GLint old_texID;
    bool offscreenRender;
    uint16_t screenCounter;

    // Per-frame performance stats
    struct PerfCounters {
        // TODO(SamNi): I don't feel comfortable with glfw in the backend. Fix?
        double      t_initial;  // BeginFrame()
        double      t_final;    // EndFrame();
    };

    // Various GL specs
    struct Specs {
        // Try to keep in alphabetical order
        int nMaxCombinedTextureImageUnits;
        int nMaxDrawBuffers;
        int nMaxElementsIndices;
        int nMaxElementsVertices;
        int nMaxTextureImageUnits;
        int nMaxVertexAttribs;
    };
    Specs mSpecs;
    PerfCounters mPerfCounts;


};

// exports
bool Startup(int w, int h) {
    pBackend = std::unique_ptr<Pimpl>(new Pimpl());
    return pBackend->Startup(w, h);
}

void Shutdown(void) {
    pBackend->Shutdown();
    pBackend.reset(nullptr);
}

void BeginFrame(void) { pBackend->BeginFrame(); }
void EndFrame(void) { pBackend->EndFrame(); }
void Resize(int w, int h) { pBackend->Resize(w, h); }
void Screenshot(void) { pBackend->Screenshot(); }

} // ~namespace