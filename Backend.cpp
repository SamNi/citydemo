// Copyright [year] <Copyright Owner>
// Goal: isolate the direct OpenGL calls, enums to the back end
#include "./Backend.h"
#include "./GL.h"
#include "./GLM.h"
#include "Shader.h"
#include <png.h>            // for writing screenshots

#pragma warning(disable : 4800)

static const int        OFFSCREEN_WIDTH =           160;
static const int        OFFSCREEN_HEIGHT =          90;
static const bool       PIXELATED =                 false;
static const uint8_t    NUM_QUEUES =                2;
static const uint8_t    QUEUE_SIZE =                8;

struct Framebuffer {
    explicit Framebuffer(int w, int h) : width(w), height(h) {
        // TODO(SamNi): I know this is suboptimal.
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTextureID);

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        // shut up, nvidia
        // http://www.opengl.org/wiki/Hardware_specifics:_NVidia?wiki/Hardware_specifics:_NVidia
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
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

struct DrawElementsIndirectCommand {
    unsigned int idxCount, instanceCount, firstIndex, baseVertex, baseInstance;
};

struct TriList {
    explicit TriList(void) : vao(0), vbo_points(0), vbo_indices(0) { }
    void Init(const glm::vec3* v, int n) {
        if (IsValid())
            return;
        glGenBuffers(1, &vbo_points);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_points);
        glBufferData(GL_ARRAY_BUFFER, 3*n*sizeof(GLfloat), glm::value_ptr(*v), GL_STATIC_DRAW);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_points);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);

        indexed = false;
        this->nVerts = nVerts;
        this->nIdx = 0;
    }
    void Init(const glm::vec3 *v, const GLubyte *indices, int nVerts, int nIndices) {
        if (IsValid())
            return;

        glGenBuffers(1, &indirect_draw_buffer);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_draw_buffer);
        glBufferData(GL_DRAW_INDIRECT_BUFFER,
            1*sizeof(DrawElementsIndirectCommand),
            NULL,
            GL_DYNAMIC_DRAW);
        DrawElementsIndirectCommand *cmd;
        cmd = (DrawElementsIndirectCommand *)glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, 1*sizeof(DrawElementsIndirectCommand), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        cmd[0].firstIndex = 0;                  // start from where
        cmd[0].idxCount = nIndices;             // number of indices
        cmd[0].baseInstance = 0;                // number of the first copy
        cmd[0].instanceCount = 1;               // how many copies
        cmd[0].baseVertex = 0;                  // initial vertex offset
        glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo_points);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_points);
        glBufferData(GL_ARRAY_BUFFER, 3*nVerts*sizeof(GLfloat), glm::value_ptr(*v), GL_STATIC_DRAW);

        glGenBuffers(1, &vbo_indices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, nIndices*sizeof(GLubyte), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        idx = indices;

        indexed = true;
        this->nVerts = nVerts;
        this->nIdx = nIndices;
    }

    void Draw(void) const {
        if (!IsValid())
            return;
        glBindVertexArray(vao);
        if (indexed) {
            glMultiDrawElementsIndirect(GL_TRIANGLES,
                GL_UNSIGNED_BYTE,
                nullptr,
                1,  // number of elements
                0   // tightly packed
                );
        } else {
            glDrawArrays(GL_TRIANGLES, 0, nVerts);
        }
        glBindVertexArray(0);
    }
    GLuint  get_vao(void) const { return vao; }
    GLuint  get_vbo(void) const { return vbo_points; }
    GLuint get_vbo_indices(void) const { return vbo_indices; }
    const GLubyte* get_indices(void) const { return idx; }
    inline bool IsValid(void) const { return vao && vbo_points && vbo_indices && idx; }

private:
    GLuint indirect_draw_buffer;
    GLuint vbo_points, vbo_indices, vao;
    const GLubyte *idx;
    int nVerts, nIdx;
    bool indexed;
};

static Framebuffer *offscreenFB = nullptr;

const uint32_t WORLD_NUM_VTX = 3;
const uint32_t WORLD_NUM_IDX = 3;
const uint32_t WORLD_NUM_INDIRECT_CMDS = 1;
const uint32_t WORLD_VTX_BUF_SIZE = WORLD_NUM_VTX*3*sizeof(GLfloat);
const uint32_t WORLD_IDX_BUF_SIZE = WORLD_NUM_IDX*sizeof(GLubyte);
const uint32_t WORLD_INDIRECT_CMD_BUF_SIZE = WORLD_NUM_INDIRECT_CMDS*sizeof(DrawElementsIndirectCommand);

struct GeometryBuffer {
    explicit GeometryBuffer(void) : mIsOpen(false) {
        LOG(LOG_TRACE, "GeometryBuffer open");

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vertex_buffer_id);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id);
        glBufferStorage(GL_ARRAY_BUFFER, WORLD_VTX_BUF_SIZE, nullptr, GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);

        glGenBuffers(1, &index_buffer_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_id);
        glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, WORLD_IDX_BUF_SIZE, nullptr, GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);

        glGenBuffers(1, &indirect_draw_buffer_id);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_draw_buffer_id);
        glBufferStorage(GL_DRAW_INDIRECT_BUFFER, WORLD_INDIRECT_CMD_BUF_SIZE, nullptr, GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }
    ~GeometryBuffer(void) {
        if (IsOpen())
            Close();

        glDeleteBuffers(1, &indirect_draw_buffer_id);
        glDeleteBuffers(1, &index_buffer_id);
        glDeleteBuffers(1, &vertex_buffer_id);
    }
    // opens the entire buffer (this is wasteful)
    void Open(void) {
        if (IsOpen()) {
            LOG(LOG_WARNING, "Redundant GeometryBuffer opening");
            return;
        }
        glBindVertexArray(vao);
        vertBuf = (GLfloat*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        idxBuf = (GLubyte*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        cmdBuf = (DrawElementsIndirectCommand*)glMapBuffer(GL_DRAW_INDIRECT_BUFFER, GL_WRITE_ONLY);
        if (NULL == vertBuf)
            LOG(LOG_WARNING, "glMapBuffer(GL_ARRAY_BUFFER,) returned null");
        if (NULL == idxBuf)
            LOG(LOG_WARNING, "glMapBuffer(GL_ELEMENT_ARRAY_BUFFER,) returned null");
        if (NULL == cmdBuf)
            LOG(LOG_WARNING, "glMapBuffer(GL_DRAW_INDIRECT_BUFFER,) returned null");
        glBindVertexArray(0);

        mIsOpen = true;
    }
    void Close(void) {
        if (!IsOpen()) {
            LOG(LOG_WARNING, "Redundant GeometryBuffer closing");
            return;
        }
        glBindVertexArray(vao);
        if (GL_TRUE != glUnmapBuffer(GL_ARRAY_BUFFER))
            LOG(LOG_WARNING, "glUnmapBuffer(GL_ARRAY_BUFFER) failed");
        if (GL_TRUE != glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER))
            LOG(LOG_WARNING, "glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER) failed");
        if (GL_TRUE != glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER))
            LOG(LOG_WARNING, "glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER) failed");
        glBindVertexArray(0);
 
        vertBuf = nullptr;
        idxBuf = nullptr;
        cmdBuf = nullptr;
        mIsOpen = false;
    }
    inline bool IsOpen(void) const { return mIsOpen; }
    inline GLuint get_vao(void) const { return vao; }
    inline GLuint get_indirect_buf_id(void) const { return indirect_draw_buffer_id; };
    inline GLfloat *getVertBufPtr(void) const { return vertBuf; }
    inline GLubyte *getIdxBufPtr(void) const { return idxBuf; }
    inline DrawElementsIndirectCommand *getCmdBufPtr(void) const { return cmdBuf; }
private:
    GLuint vertex_buffer_id;
    GLuint index_buffer_id;
    GLuint indirect_draw_buffer_id;
    GLuint vao;
    GLfloat *vertBuf;
    GLubyte *idxBuf;
    DrawElementsIndirectCommand *cmdBuf;
    bool mIsOpen;
};

struct Backend::Impl {
    inline void ClearPerformanceCounters(void) { memset(&mPerfCounts, NULL, sizeof(PerfCounters)); }
    inline void QueryHardwareSpecs(void) {
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

        mSpecs.extensions.resize(mSpecs.nExtensions, "<null>");

        for (int i = 0;i < mSpecs.nExtensions;++i) {
            static const GLubyte *c;
            c = glGetStringi(GL_EXTENSIONS, i);
            LOG(LOG_TRACE, "extension: %s", c);
            mSpecs.extensions[i] = (const char*)c;
        }

    }
    GeometryBuffer geom_buf;

    bool Startup(int w, int h) {
        // misc. defaults
        current_screen_width = w;
        current_screen_height = h;
        screenshotCounter = 0;
        offscreenRender = PIXELATED;

        ClearPerformanceCounters();
        QueryHardwareSpecs();

        // Allocate world geometry buffers

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

    void AddTris(void) {
        static GLfloat *vtx = nullptr;
        static GLubyte *idx = nullptr;
        static DrawElementsIndirectCommand *cmd = nullptr;

        if (vtx || idx || cmd)
            return;

        geom_buf.Open();
        vtx = (GLfloat*)geom_buf.getVertBufPtr();
        idx = (GLubyte*)geom_buf.getIdxBufPtr();
        cmd = (DrawElementsIndirectCommand*)geom_buf.getCmdBufPtr();
        {
            vtx[0] = -1.0f;
            vtx[1] = -1.0f;
            vtx[2] =  0.0f;

            vtx[3] = +1.0f;
            vtx[4] = -1.0f;
            vtx[5] =  0.0f;

            vtx[6] =  0.0f;
            vtx[7] = +1.0f;
            vtx[8] =  0.0f;

            idx[0] = 0;
            idx[1] = 1;
            idx[2] = 2;

            cmd[0].baseVertex = 0;
            cmd[0].baseInstance = 0;
            cmd[0].firstIndex = 0;
            cmd[0].idxCount = 3;
            cmd[0].instanceCount = 1;
        }
        geom_buf.Close();
    }
    void EndFrame(void) {
        glBindVertexArray(geom_buf.get_vao());
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_BYTE, 0, 1, 0);
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
        int             nExtensions;
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
        std::vector<const char*> extensions;
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
void Backend::AddTris(void) { mImpl->AddTris(); }
