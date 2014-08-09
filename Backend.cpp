// Copyright [year] <Copyright Owner>
// Goal: isolate the direct OpenGL calls, enums to the back end
#include "./Backend.h"
#include "./GL.h"
#include "./GLM.h"
#include "Shader.h"
#include <png.h>            // for writing screenshots

#pragma warning(disable : 4800)

static const int        OFFSCREEN_WIDTH =           1600;
static const int        OFFSCREEN_HEIGHT =          900;
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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

// adjust these to taste
// SoA : structures of arrays
const uint32_t WORLD_NUM_VTX = (1 << 20);
const uint32_t WORLD_NUM_COLOR = WORLD_NUM_VTX;
const uint32_t WORLD_NUM_TEXCOORD = WORLD_NUM_VTX;
const uint32_t WORLD_NUM_NORMAL = WORLD_NUM_VTX;
const uint32_t WORLD_NUM_IDX = (1 << 18);
const uint32_t WORLD_NUM_INDIRECT_CMDS = (1 << 10);

const uint32_t WORLD_VTX_BUF_SIZE = WORLD_NUM_VTX*3*sizeof(GLfloat); // 3 floating points for x, y, z
const uint32_t WORLD_COLOR_BUF_SIZE = WORLD_NUM_COLOR*4*sizeof(GLubyte); // one byte per component, four components
const uint32_t WORLD_TEXCOORD_BUF_SIZE = WORLD_NUM_COLOR*2*sizeof(GLushort); // 1 ushort per texture coordinate
const uint32_t WORLD_NORMAL_BUF_SIZE = WORLD_NUM_NORMAL*sizeof(uint32_t); // a single GL_UNSIGNED_INT_10_10_10_2
const uint32_t WORLD_IDX_BUF_SIZE = WORLD_NUM_IDX*sizeof(GLushort); // a single ushort for the index
const uint32_t WORLD_INDIRECT_CMD_BUF_SIZE = WORLD_NUM_INDIRECT_CMDS*sizeof(DrawElementsIndirectCommand);

const GLenum GEOMETRY_BUF_FLAGS = GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT;

struct RGBA {
    GLubyte r, g, b, a;
};

struct TexCoord32 {
    GLushort u, v;
};

struct GeometryBuffer {
    enum GeometryBufferID {
        VERTEX_ATTR_POSITION = 0,
        VERTEX_ATTR_COLOR,
        VERTEX_ATTR_TEXCOORD,
        VERTEX_ATTR_NORMAL,
        VERTEX_INDEX,
        INDIRECT_DRAW_CMD
    };
    static const int NUM_GEOM_BUF = 6;
    explicit GeometryBuffer(void) : mIsOpen(false) {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(NUM_GEOM_BUF, vertAttrID);
        glBindBuffer(GL_ARRAY_BUFFER, vertAttrID[VERTEX_ATTR_POSITION]);
        glBufferStorage(GL_ARRAY_BUFFER, WORLD_VTX_BUF_SIZE, nullptr, GEOMETRY_BUF_FLAGS);
        glVertexAttribPointer(VERTEX_ATTR_POSITION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        
        glBindBuffer(GL_ARRAY_BUFFER, vertAttrID[VERTEX_ATTR_COLOR]);
        glBufferStorage(GL_ARRAY_BUFFER, WORLD_COLOR_BUF_SIZE, nullptr, GEOMETRY_BUF_FLAGS);
        glVertexAttribPointer(VERTEX_ATTR_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, nullptr);

        glBindBuffer(GL_ARRAY_BUFFER, vertAttrID[VERTEX_ATTR_TEXCOORD]);
        glBufferStorage(GL_ARRAY_BUFFER, WORLD_TEXCOORD_BUF_SIZE, nullptr, GEOMETRY_BUF_FLAGS);
        glVertexAttribPointer(VERTEX_ATTR_TEXCOORD, 2, GL_UNSIGNED_SHORT, GL_TRUE, 0, nullptr);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertAttrID[VERTEX_INDEX]);
        glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, WORLD_IDX_BUF_SIZE, nullptr, GEOMETRY_BUF_FLAGS);

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, vertAttrID[INDIRECT_DRAW_CMD]);
        glBufferStorage(GL_DRAW_INDIRECT_BUFFER, WORLD_INDIRECT_CMD_BUF_SIZE, nullptr, GEOMETRY_BUF_FLAGS);

        glEnableVertexAttribArray(VERTEX_ATTR_POSITION);
        glEnableVertexAttribArray(VERTEX_ATTR_COLOR);
        glEnableVertexAttribArray(VERTEX_ATTR_TEXCOORD);
        glBindVertexArray(0);

        LOG(LOG_TRACE, "GeometryBuffer opened");
        LOG(LOG_TRACE, "Vertex position buffer %.2lf mB (%u vertices)", WORLD_VTX_BUF_SIZE / (1024.0*1024.0), WORLD_NUM_VTX);
        LOG(LOG_TRACE, "Vertex color buffer %.2lf mB (%u colors)", WORLD_COLOR_BUF_SIZE / (1024.0*1024.0), WORLD_NUM_VTX);
        LOG(LOG_TRACE, "Vertex texcoord buffer %.2lf mB (%u texCoords)", WORLD_TEXCOORD_BUF_SIZE / (1024.0*1024.0), WORLD_NUM_VTX);
        LOG(LOG_TRACE, "Index buffer %.2lf mB (%u indices)", WORLD_IDX_BUF_SIZE / (1024.0*1024.0), WORLD_NUM_IDX);
        LOG(LOG_TRACE, "Indirect cmd buffer %.2lf kB (%u commands)", WORLD_INDIRECT_CMD_BUF_SIZE / (1024.0), WORLD_NUM_INDIRECT_CMDS);
        LOG(LOG_TRACE, "Total OGL buffer size %.2lf mB", (WORLD_VTX_BUF_SIZE+WORLD_COLOR_BUF_SIZE+WORLD_TEXCOORD_BUF_SIZE+WORLD_IDX_BUF_SIZE+WORLD_INDIRECT_CMD_BUF_SIZE)/(1024.0*1024.0));
        LOG(LOG_TRACE, "Application host size %u B", sizeof(GeometryBuffer));
    }
    ~GeometryBuffer(void) {
        if (IsOpen())
            Close();

        glDeleteBuffers(NUM_GEOM_BUF, vertAttrID);
        glDeleteVertexArrays(1, &vao);
    }
    // opens the entire buffer (this is wasteful)
    void Open(void) {
        if (IsOpen()) {
            LOG(LOG_WARNING, "Redundant GeometryBuffer opening");
            return;
        }
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vertAttrID[VERTEX_ATTR_POSITION]);
        vertBuf = (GLfloat*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        glBindBuffer(GL_ARRAY_BUFFER, vertAttrID[VERTEX_ATTR_COLOR]);
        colBuf = (RGBA*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        glBindBuffer(GL_ARRAY_BUFFER, vertAttrID[VERTEX_ATTR_TEXCOORD]);
        texCoordBuf = (TexCoord32*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        idxBuf = (GLushort*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
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
        glBindBuffer(GL_ARRAY_BUFFER, vertAttrID[VERTEX_ATTR_POSITION]);
        if (GL_TRUE != glUnmapBuffer(GL_ARRAY_BUFFER))
            LOG(LOG_WARNING, "glUnmapBuffer(GL_ARRAY_BUFFER) failed on vertex_buffer_id");
        glBindBuffer(GL_ARRAY_BUFFER, vertAttrID[VERTEX_ATTR_COLOR]);
        if (GL_TRUE != glUnmapBuffer(GL_ARRAY_BUFFER))
            LOG(LOG_WARNING, "glUnmapBuffer(GL_ARRAY_BUFFER) failed on color_buffer_id");
        glBindBuffer(GL_ARRAY_BUFFER, vertAttrID[VERTEX_ATTR_TEXCOORD]);
        if (GL_TRUE != glUnmapBuffer(GL_ARRAY_BUFFER))
            LOG(LOG_WARNING, "glUnmapBuffer(GL_ARRAY_BUFFER) failed on texcoord_buffer_id");
        if (GL_TRUE != glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER))
            LOG(LOG_WARNING, "glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER) failed");
        if (GL_TRUE != glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER))
            LOG(LOG_WARNING, "glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) failed");
        glBindVertexArray(0);
 
        vertBuf = nullptr;
        colBuf = nullptr;
        idxBuf = nullptr;
        cmdBuf = nullptr;
        mIsOpen = false;
    }
    inline bool IsOpen(void) const { return mIsOpen; }
    inline GLuint get_vao(void) const { return vao; }
    inline GLuint get_indirect_buf_id(void) const { return vertAttrID[INDIRECT_DRAW_CMD]; };
    inline GLfloat *getVertBufPtr(void) const { return vertBuf; }
    inline RGBA *getColBufPtr(void) const { return colBuf; }
    inline TexCoord32 *getTexCoordBufPtr(void) const { return texCoordBuf; }

    inline GLushort *getIdxBufPtr(void) const { return idxBuf; }
    inline DrawElementsIndirectCommand *getCmdBufPtr(void) const { return cmdBuf; }

private:
    GLuint vertAttrID[NUM_GEOM_BUF];
    GLuint vao;

    GLfloat *vertBuf;
    RGBA *colBuf;
    TexCoord32 *texCoordBuf;

    GLushort *idxBuf;
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
        static glm::vec3 *vtx = nullptr;
        static RGBA *col = nullptr;
        static TexCoord32 *tex = nullptr;
        static GLushort *idx = nullptr;
        static DrawElementsIndirectCommand *cmd = nullptr;

        if (vtx || tex || col || idx || cmd)
            return;

        geom_buf.Open();
        vtx = (glm::vec3*)geom_buf.getVertBufPtr();
        col = (RGBA*)geom_buf.getColBufPtr();
        tex = (TexCoord32*)geom_buf.getTexCoordBufPtr();

        idx = (GLushort*)geom_buf.getIdxBufPtr();
        cmd = (DrawElementsIndirectCommand*)geom_buf.getCmdBufPtr();
        {
            vtx[0] = glm::vec3(-1.0f, 0.0f, 0.0f);
            vtx[1] = glm::vec3(+1.0f, 0.0f, 0.0f);
            vtx[2] = glm::vec3( 0.0f,-1.0f, 0.0f);
            vtx[3] = glm::vec3( 0.0f,+1.0f, 0.0f);

            col[0].r = 255;
            col[0].g = 0;
            col[0].b = 0;
            col[0].a = 255;
            col[1].r = 0;
            col[1].g = 0;
            col[1].b = 255;
            col[1].a = 255;
            col[2].r = 0;
            col[2].g = 0;
            col[2].b = 0;
            col[2].a = 0;
            col[3].r = 255;
            col[3].g = 0;
            col[3].b = 255;
            col[3].a = 255;

            tex[0].u = 0;
            tex[0].v = 0;
            tex[1].u = 65535;
            tex[1].v = 65535;
            tex[2].u = 65535;
            tex[2].v = 0;
            tex[3].u = 0;
            tex[3].v = 65535;

            idx[0] = 0;
            idx[1] = 1;
            idx[2] = 2;
            idx[3] = 0;
            idx[4] = 1;
            idx[5] = 3;

            cmd[0].baseVertex = 0;
            cmd[0].baseInstance = 0;
            cmd[0].firstIndex = 0;
            cmd[0].idxCount = 6;
            cmd[0].instanceCount = 1;

        }
        geom_buf.Close();
    }
    void EndFrame(void) {
        glBindVertexArray(geom_buf.get_vao());
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, 0, 1, 0);
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
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

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
        glBindVertexArray(0);
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
