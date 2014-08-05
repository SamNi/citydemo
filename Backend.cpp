// Copyright [year] <Copyright Owner>
// Goal: isolate the direct OpenGL calls, enums to the back end
#include "./Backend.h"
#include "./GL.h"
#include "./GLM.h"
#include "Shader.h"
#include <png.h>            // for writing screenshots

#pragma warning(disable : 4800)

static const int        OFFSCREEN_WIDTH =           256;
static const int        OFFSCREEN_HEIGHT =          256;
static const bool       PIXELATED =                 true;

struct Framebuffer {
    explicit Framebuffer(int w, int h) : width(w), height(h) {
        // TODO(SamNi): I know this is suboptimal.
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTextureID);

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        checkGL();

        glGenFramebuffers(1, &framebufferID);
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureID, 0);
        checkGL();

        glBindTexture(GL_TEXTURE_2D, oldTextureID);
        checkGL();
    }
    void Bind(void) const {
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
        glBindTexture(GL_TEXTURE_2D, oldTextureID);
        glViewport(0, 0, Framebuffer::width, Framebuffer::height);
        checkGL();
    }
    void Blit(int w, int h) const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);       // switch to the visible render buffer
        glDisable(GL_DEPTH_TEST);
        glClear(GL_DEPTH_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glViewport(0, 0, w, h);
        Backend::DrawFullscreenQuad();
    }
    GLuint          framebufferID;
    GLuint          textureID;
    GLint           oldTextureID;
    uint16_t        width, height;
};

static Framebuffer *offscreenFB = nullptr;

struct Backend::Impl {
    inline void ClearPerformanceCounters(void) { memset(&mPerfCounts, NULL, sizeof(PerfCounters)); }
    inline void QueryHardwareSpecs(void) {
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
    }
    bool Startup(int w, int h) {
        current_screen_width = w;
        current_screen_height = h;
        screenshotCounter = 0;

        ClearPerformanceCounters();
        QueryHardwareSpecs();

        // Allocate things, prepare VBOs,*/
        offscreenRender = PIXELATED;
        return true;
    }
    void Shutdown(void) {
        // glDelete* calls should go here
    }

    void BeginFrame(void) {
        ClearPerformanceCounters();
        if (offscreenRender) {
            if (nullptr == offscreenFB)
                offscreenFB = new Framebuffer(OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
            offscreenFB->Bind();    
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    void EndFrame(void) {
        if (offscreenRender)
            offscreenFB->Blit(current_screen_width, current_screen_height);
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

        sprintf(filename, "screenshot%05u.png", screenshotCounter++);

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
        static GLint progHandle, loc, loc2;
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

            glGetIntegerv(GL_CURRENT_PROGRAM, &progHandle);
            loc = glGetUniformLocation(progHandle, "modelView");
            loc2 = glGetUniformLocation(progHandle, "projection");
            firstTime = false;
        } else
            glBindVertexArray(vao);

        static float angle = 0.0f;
        static glm::mat4 modelView;
        static glm::mat4 projection;

        modelView = glm::rotate(angle, glm::vec3(0.0f,0.0f,1.0f));
        modelView *= glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        projection = glm::perspective(glm::radians(75.0f), (float)current_screen_width/current_screen_height, 0.01f, 100.0f);
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(modelView));
        glUniformMatrix4fv(loc2, 1, GL_FALSE, glm::value_ptr(projection));
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        angle += 0.01f;
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

    bool offscreenRender;
    uint16_t screenshotCounter;

    // Per-frame performance stats
    struct PerfCounters {
        /// TBD
        int poop;
    };
    PerfCounters mPerfCounts;

    // Various GL specs
    struct Specs {
        // Try to keep in alphabetical order
        int             nMaxCombinedTextureImageUnits;
        int             nMaxDrawBuffers;
        int             nMaxElementsIndices;
        int             nMaxElementsVertices;
        int             nMaxFragmentUniformBlocks;
        int             nMaxGeometryUniformBlocks;
        int             nMaxTextureImageUnits;
        int             nMaxUniformBufferBindings;
        int             nMaxVertexAttribs;
        int             nMaxVertexUniformBlocks;

        const GLubyte*  renderer;
        const GLubyte*  vendor;
        const GLubyte*  version;
    };
    Specs mSpecs;
};

std::unique_ptr<Backend::Impl> Backend::mImpl = nullptr;

// public interface
bool Backend::Startup(int w, int h) {
    mImpl = std::unique_ptr<Impl>(new Impl());
    return mImpl->Startup(w, h);
}

void Backend::Shutdown(void) {
    mImpl->Shutdown();
    mImpl.reset(nullptr);
}

void Backend::BeginFrame(void) { mImpl->BeginFrame(); }
void Backend::EndFrame(void) { mImpl->EndFrame(); }
void Backend::Resize(int w, int h) { mImpl->Resize(w, h); }
void Backend::Screenshot(void) { mImpl->Screenshot(); }
void Backend::DrawFullscreenQuad(void) { mImpl->DrawFullscreenQuad(); }