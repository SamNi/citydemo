// Copyright [year] <Copyright Owner>
// Goal: isolate the direct OpenGL calls, enums to the back end
#include "./Backend.h"
#include "./GL.h"
#include "./GLM.h"
#include "Shader.h"
#include <png.h>            // for writing screenshots

#pragma warning(disable : 4800)

static const int        OFFSCREEN_WIDTH =           256;
static const int        OFFSCREEN_HEIGHT =          240;
static const bool       PIXELATED =                 false;
static const uint8_t    NUM_QUEUES =                2;
static const uint8_t    QUEUE_SIZE =                8;

inline void wipe_memory(void *dest, size_t n) { memset(dest, NULL, n); }

struct Framebuffer {
    explicit Framebuffer(uint16_t w, uint16_t h) : width(w), height(h) {
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
        glClear(GL_COLOR_BUFFER_BIT);
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

struct RGBA {
    GLubyte r, g, b, a;
    RGBA(void) : r(0),g(0),b(0),a(0) { }
    RGBA(const RGBA& rhs) : r(rhs.r), g(rhs.g), b(rhs.b), a(rhs.a) { }
    RGBA(GLubyte _r, GLubyte _g, GLubyte _b, GLubyte _a) : r(_r), g(_g), b(_b), a(_a) { }
};

struct TexCoord32 {
    GLushort u, v;
};

// for use with GL_UNSIGNED_INT_2_10_10_10_REV
inline GLuint NormalPack(const glm::vec4& v) {
    return ((GLuint)(1023.0f*v.x) << 0) | 
        ((GLuint)(1023.0f*v.y) << 10) | 
        ((GLuint)(1023.0f*v.z) << 20) | 
        ((GLuint)(3.0f*v.a) << 30);
}

struct SurfaceTriangles {
    SurfaceTriangles(void) { 
        memset(this, NULL, sizeof(SurfaceTriangles)); 
    }
    SurfaceTriangles(const uint32_t nv, const uint32_t nidx) {
        assert(nv && nidx);
        memset(this, NULL, sizeof(SurfaceTriangles));
        vertices = new glm::vec3[nv];
        indices = new GLushort[nidx];
        nVertices = nv;
        nIndices = nidx;
    }
    ~SurfaceTriangles(void) {
        delete[] vertices;
        delete[] colors;
        delete[] texture_coordinates;
        delete[] normals;
        delete[] indices;
    }
    inline uint32_t GetSize(void) const {
        uint32_t ret = 0;
        if (nullptr != vertices)
            ret += sizeof(glm::vec4)*nVertices;
        if (nullptr != colors)
            ret += sizeof(glm::vec4)*nVertices;
        if (nullptr != texture_coordinates)
            ret += sizeof(glm::vec2)*nVertices;
        if (nullptr != normals)
            ret += sizeof(GLuint)*nVertices;
        if (nullptr != indices)
            ret += sizeof(GLushort)*nIndices;
        return ret;
    }
    uint32_t nVertices;
    glm::vec3 *vertices;

    // if there are colors (colors != NULL), it is assumed
    // that the number of them is the same as the number
    // of vertices
    // uint32_t nColors;
    RGBA *colors;

    // ditto for texture coordinates
    // uint32_t nTextureCoordinates;
    glm::u16vec2 *texture_coordinates;

    // ditto for normals
    GLuint *normals;            // store as 2_10_10_10_REV, use NormalPack()

    // however, not true for indices
    uint32_t nIndices;
    GLushort *indices;
};

static const int NUM_TRIANGLES = 8500;
SurfaceTriangles st(3*NUM_TRIANGLES, 3*NUM_TRIANGLES);

struct GeometryBuffer {
    /* brainstorm, thoughts, freeform
    what should a draw queue have?
        render commands
            what constitutes a render command?
                something you can feed into glMultiDrawElementsIndirect()
        the number of render commands
        "open" and "closed" states
            "open", the client is free to write to it. This is the in between glMapBufferRange() and glUnmapBuffer()
                in the future, possibly have compute shaders write directly to the GPU to avoid the roundtrip
            "closed", client does not touch it. The GPU now reads from it.

    You should have two queues that you swap between
        At all times, one will be open and the other closed

    A render command, as glMultiDrawElementsIndirect() sees it
        number of indices
        number of instances
        the offset of the index considered to be the zeroth
        the offset of the vertex considered to be the zeroth
        the number of the first instance (typically zero)

    idea: preallocate giant world geometry buffer upfront and manage it
        managing this giant world geometry buffer would require some metadata, structures
            all client side, of course. the buffer itself would be on the GPU
        you will probably need to use these functions to manipulate the buffer on the GPU
            glMapBuffer
            glMapBufferRange
            glUnmapBuffer
            glBufferData
            glBufferSubData
            glBufferStorage     (for immutable)
            glClearBufferData
            glClearBufferSubData
            glCopyBufferSubData
            glDeleteBuffers
            glFlushMappedBufferRange
        Expect to be using write-only immutable storage a lot
            GL_MAP_WRITE to upload world geometry from the client
            GL_DYNAMIC_STORAGE to modify small subsets of the geometry
            Maybe the GL_PERSISTENT.... maybe...
        Watch out for synchronization issues! You might need
            glMemoryBarrier
            glFlushMappedBufferRange    */
    // adjust these to taste
    // SoA : structures of arrays
    static const uint32_t WORLD_NUM_VTX = (1 << 20);
    static const uint32_t WORLD_NUM_COLOR = WORLD_NUM_VTX;
    static const uint32_t WORLD_NUM_TEXCOORD = WORLD_NUM_VTX;
    static const uint32_t WORLD_NUM_NORMAL = WORLD_NUM_VTX;
    static const uint32_t WORLD_NUM_IDX = (1 << 18);
    static const uint32_t WORLD_NUM_INDIRECT_CMDS = (1 << 10);

    static const uint32_t WORLD_VTX_BUF_SIZE = WORLD_NUM_VTX*3*sizeof(GLfloat); // 3 floating points for x, y, z
    static const uint32_t WORLD_COLOR_BUF_SIZE = WORLD_NUM_COLOR*4*sizeof(GLubyte); // one byte per component, four components
    static const uint32_t WORLD_TEXCOORD_BUF_SIZE = WORLD_NUM_COLOR*2*sizeof(GLushort); // 1 ushort per texture coordinate
    static const uint32_t WORLD_NORMAL_BUF_SIZE = WORLD_NUM_NORMAL*sizeof(uint32_t); // a single GL_UNSIGNED_INT_10_10_10_2
    static const uint32_t WORLD_IDX_BUF_SIZE = WORLD_NUM_IDX*sizeof(GLushort); // a single ushort for the index
    static const uint32_t WORLD_INDIRECT_CMD_BUF_SIZE = WORLD_NUM_INDIRECT_CMDS*sizeof(DrawElementsIndirectCommand);

    static const GLenum GEOMETRY_BUF_STORAGE_FLAGS = GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT;
    enum GeometryBufferID {
        VERTEX_ATTR_POSITION = 0,
        VERTEX_ATTR_COLOR,
        VERTEX_ATTR_TEXCOORD,
        VERTEX_ATTR_NORMAL,
        VERTEX_INDEX,
        INDIRECT_DRAW_CMD,
        GEO_BUF_ID_FINAL        // unused
    };
    static const int NUM_GEOM_BUF = GEO_BUF_ID_FINAL - VERTEX_ATTR_POSITION;

    explicit GeometryBuffer(void) : mIsOpen(false), current_index(0) {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(NUM_GEOM_BUF, buf_id);
        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_POSITION]);
        glBufferStorage(GL_ARRAY_BUFFER, WORLD_VTX_BUF_SIZE, nullptr, GEOMETRY_BUF_STORAGE_FLAGS);
        glVertexAttribPointer(VERTEX_ATTR_POSITION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        
        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_COLOR]);
        glBufferStorage(GL_ARRAY_BUFFER, WORLD_COLOR_BUF_SIZE, nullptr, GEOMETRY_BUF_STORAGE_FLAGS);
        glVertexAttribPointer(VERTEX_ATTR_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, nullptr);

        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_TEXCOORD]);
        glBufferStorage(GL_ARRAY_BUFFER, WORLD_TEXCOORD_BUF_SIZE, nullptr, GEOMETRY_BUF_STORAGE_FLAGS);
        glVertexAttribPointer(VERTEX_ATTR_TEXCOORD, 2, GL_UNSIGNED_SHORT, GL_TRUE, 0, nullptr);

        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_NORMAL]);
        glBufferStorage(GL_ARRAY_BUFFER, WORLD_NORMAL_BUF_SIZE, nullptr, GEOMETRY_BUF_STORAGE_FLAGS);
        glVertexAttribPointer(VERTEX_ATTR_NORMAL, 4, GL_UNSIGNED_INT_2_10_10_10_REV, GL_TRUE, 0, nullptr);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf_id[VERTEX_INDEX]);
        glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, WORLD_IDX_BUF_SIZE, nullptr, GEOMETRY_BUF_STORAGE_FLAGS);

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buf_id[INDIRECT_DRAW_CMD]);
        glBufferStorage(GL_DRAW_INDIRECT_BUFFER, WORLD_INDIRECT_CMD_BUF_SIZE, nullptr, GEOMETRY_BUF_STORAGE_FLAGS);

        glEnableVertexAttribArray(VERTEX_ATTR_POSITION);
        glEnableVertexAttribArray(VERTEX_ATTR_COLOR);
        glEnableVertexAttribArray(VERTEX_ATTR_TEXCOORD);
        glEnableVertexAttribArray(VERTEX_ATTR_NORMAL);
        glBindVertexArray(0);

        LOG(LOG_TRACE, "GeometryBuffer opened");
        LOG(LOG_TRACE, "Vertex position buffer %.2lf mB (%u vertices)", WORLD_VTX_BUF_SIZE / (1024.0*1024.0), WORLD_NUM_VTX);
        LOG(LOG_TRACE, "Vertex color buffer %.2lf mB (%u colors)", WORLD_COLOR_BUF_SIZE / (1024.0*1024.0), WORLD_NUM_VTX);
        LOG(LOG_TRACE, "Vertex texcoord buffer %.2lf mB (%u texCoords)", WORLD_TEXCOORD_BUF_SIZE / (1024.0*1024.0), WORLD_NUM_VTX);
        LOG(LOG_TRACE, "Vertex normal buffer %.2lf mB (%u texCoords)", WORLD_NORMAL_BUF_SIZE / (1024.0*1024.0), WORLD_NUM_VTX);
        LOG(LOG_TRACE, "Index buffer %.2lf mB (%u indices)", WORLD_IDX_BUF_SIZE / (1024.0*1024.0), WORLD_NUM_IDX);
        LOG(LOG_TRACE, "Indirect cmd buffer %.2lf kB (%u commands)", WORLD_INDIRECT_CMD_BUF_SIZE / (1024.0), WORLD_NUM_INDIRECT_CMDS);
        LOG(LOG_TRACE, "Total OGL buffer size %.2lf mB", (WORLD_VTX_BUF_SIZE+WORLD_COLOR_BUF_SIZE+WORLD_TEXCOORD_BUF_SIZE+WORLD_NORMAL_BUF_SIZE+WORLD_IDX_BUF_SIZE+WORLD_INDIRECT_CMD_BUF_SIZE)/(1024.0*1024.0));
        LOG(LOG_TRACE, "Application host size %u B", sizeof(GeometryBuffer));
    }
    ~GeometryBuffer(void) {
        if (IsOpen())
            Close();

        glDeleteBuffers(NUM_GEOM_BUF, buf_id);
        glDeleteVertexArrays(1, &vao);
    }
    // opens the entire buffer (this is wasteful)
    void Open(void) {
        if (IsOpen()) {
            LOG(LOG_WARNING, "Redundant GeometryBuffer opening");
            return;
        }
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_POSITION]);
        vertBuf = (glm::vec3*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_COLOR]);
        colBuf = (RGBA*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_TEXCOORD]);
        texCoordBuf = (glm::u16vec2*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_NORMAL]);
        normalBuf = (GLuint*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        idxBuf = (GLushort*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        cmdBuf = (DrawElementsIndirectCommand*)glMapBuffer(GL_DRAW_INDIRECT_BUFFER, GL_WRITE_ONLY);
        if (!(vertBuf && colBuf && texCoordBuf))
            LOG(LOG_WARNING, "glMapBuffer(GL_ARRAY_BUFFER,) returned null");
        if (NULL == idxBuf)
            LOG(LOG_WARNING, "glMapBuffer(GL_ELEMENT_ARRAY_BUFFER,) returned null");
        if (NULL == cmdBuf)
            LOG(LOG_WARNING, "glMapBuffer(GL_DRAW_INDIRECT_BUFFER,) returned null");

        mIsOpen = true;
    }
    void Close(void) {
        if (!IsOpen()) {
            LOG(LOG_WARNING, "Redundant GeometryBuffer closing");
            return;
        }
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_POSITION]);
        if (GL_TRUE != glUnmapBuffer(GL_ARRAY_BUFFER))
            LOG(LOG_WARNING, "glUnmapBuffer(GL_ARRAY_BUFFER) failed on VERTEX_ATTR_POSITION");
        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_COLOR]);
        if (GL_TRUE != glUnmapBuffer(GL_ARRAY_BUFFER))
            LOG(LOG_WARNING, "glUnmapBuffer(GL_ARRAY_BUFFER) failed on VERTEX_ATTR_COLOR");
        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_TEXCOORD]);
        if (GL_TRUE != glUnmapBuffer(GL_ARRAY_BUFFER))
            LOG(LOG_WARNING, "glUnmapBuffer(GL_ARRAY_BUFFER) failed on VERTEX_ATTR_TEXCOORD");
        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_NORMAL]);
        if (GL_TRUE != glUnmapBuffer(GL_ARRAY_BUFFER))
            LOG(LOG_WARNING, "glUnmapBuffer(GL_ARRAY_BUFFER) failed on VERTEX_ATTR_NORMAL");
        if (GL_TRUE != glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER))
            LOG(LOG_WARNING, "glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER) failed");
        if (GL_TRUE != glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER))
            LOG(LOG_WARNING, "glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) failed");
 
        vertBuf = nullptr;
        colBuf = nullptr;
        texCoordBuf = nullptr;
        normalBuf = nullptr;
        idxBuf = nullptr;
        cmdBuf = nullptr;
        mIsOpen = false;
    }
    inline bool IsOpen(void) const { return mIsOpen; }

    // caller's responsibility to keep track of the location of the surface
    uint32_t AddSurface(const SurfaceTriangles& st) {
        assert(st.nVertices != 0);
        assert(st.vertices != nullptr);
        assert(st.indices != nullptr);
        assert(IsOpen());

        static uint32_t i;
        auto vtx = get_vertex_buffer_pointer();
        auto col = get_color_buffer_ptr();
        auto tex = get_texture_coordinate_buffer_pointer();
        auto normals = get_normal_buffer_ptr();
        auto idx = get_index_buffer_ptr();

        for (i = 0;i < st.nVertices;++i) {
            vtx[i] = st.vertices[i];
            col[i] = (st.colors != nullptr) ? st.colors[i] : RGBA(255,255,255,255);
            tex[i] = (st.texture_coordinates != nullptr) ? st.texture_coordinates[i] : glm::u16vec2(0, 0);
            normals[i] = (st.normals != nullptr) ? st.normals[i] : NormalPack(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
        }

        for (i = 0;i < st.nIndices;++i)
            idx[i] = st.indices[i];

        current_index  += st.nIndices;
        return current_index - st.nIndices;
    }
    void AddDrawCommand(uint32_t offset, uint32_t nIndices) {
        assert (IsOpen());

        auto command_buffer = get_command_buffer_ptr();
        auto& cmd = command_buffer[cmd_queues.get_size()];

        cmd.firstIndex = offset;
        cmd.idxCount = nIndices;
        cmd.baseVertex = 0;
        cmd.baseInstance = 0;
        cmd.instanceCount = 1;

        cmd_queues.Push();
    }

    inline GLuint get_vao(void) const { return vao; }
    inline GLuint get_indirect_buffer_id(void) const { return buf_id[INDIRECT_DRAW_CMD]; };

    inline glm::vec3 *get_vertex_buffer_pointer(void) const { return vertBuf; }
    inline RGBA *get_color_buffer_ptr(void) const { return colBuf; }
    inline glm::u16vec2 *get_texture_coordinate_buffer_pointer(void) const { return texCoordBuf; }
    inline GLuint *get_normal_buffer_ptr(void) const { return normalBuf; }

    inline GLushort *get_index_buffer_ptr(void) const { return idxBuf; }
    inline DrawElementsIndirectCommand *get_command_buffer_ptr(void) const { return cmdBuf; }

    // only manages offsets into buf_id[INDIRECT_DRAW_CMD]
    struct CommandQueues {
        static const uint8_t NUM_PARTITIONS = 2;
        static const uint16_t OFFSET = WORLD_INDIRECT_CMD_BUF_SIZE / NUM_PARTITIONS;
        static const uint16_t PARTITION_SIZE = OFFSET;
        explicit CommandQueues(void) {
            current = 0;
            top[0] = 0;
            top[1] = 0;
            base[0] = 0*PARTITION_SIZE;
            base[1] = 1*PARTITION_SIZE;
        }
        void Push(void) {
            if (top[current] == PARTITION_SIZE) {
                LOG(LOG_WARNING, "CommandQueue topped out at %u", top);
                return;
            }
            ++top[current];
        }
        void Pop(void) {
            if (top == 0) {
                LOG(LOG_WARNING, "CommandQueue underflow");
                return;
            }
            --top[current];
        }
        inline void Swap(void) { current = (current+1)%2; }
        inline void Clear(void) { top[current] = 0; }
        inline uint16_t get_size(void) { return top[current]; }
        uint16_t top[2];
        uint16_t base[2];
        uint8_t current;
    };

    CommandQueues cmd_queues;

private:
    GLuint buf_id[NUM_GEOM_BUF];
    GLuint vao;

    glm::vec3 *vertBuf;
    RGBA *colBuf;
    glm::u16vec2 *texCoordBuf;
    GLuint *normalBuf;

    GLushort *idxBuf;
    DrawElementsIndirectCommand *cmdBuf;

    uint32_t current_index;

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
        geom_buf.cmd_queues.Clear();
    }

    uint32_t loc;

    void AddTris(void) {
        static bool firstTime = true;
        if (false == firstTime)
            return;
        firstTime = false;
        GLushort i;
        for (i = 0;i < 3*NUM_TRIANGLES;i += 3) {
            glm::vec3 v0(uniform(-1.0f, 1.0f), uniform(-1.0f, 1.0f), uniform(-1.0f, 1.0f));
            glm::vec3 v1(v0 + glm::vec3(uniform(-0.03f, 0.03f), uniform(-0.03f, 0.03f), uniform(-0.03f, 0.03f)));
            glm::vec3 v2(v0 + glm::vec3(uniform(-0.03f, 0.03f), uniform(-0.03f, 0.03f), uniform(-0.03f, 0.03f)));

            st.vertices[i+0] = v0;
            st.vertices[i+1] = v1;
            st.vertices[i+2] = v2;
            st.indices[i+0] = i+0;
            st.indices[i+1] = i+1;
            st.indices[i+2] = i+2;
        }
        mImpl->geom_buf.Open();
        loc = mImpl->geom_buf.AddSurface(st);
        mImpl->geom_buf.Close();

    }
    void EndFrame(void) {
        geom_buf.Open();
        glBindVertexArray(geom_buf.get_vao());
        mImpl->geom_buf.AddDrawCommand(loc, st.nIndices);
        geom_buf.Close();
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, nullptr, geom_buf.cmd_queues.get_size(), 0);
        if (offscreenRender)
            offscreenFB->Blit(current_screen_width, current_screen_height);
        geom_buf.cmd_queues.Swap();
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
