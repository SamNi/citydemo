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

typedef glm::u8vec4 RGBA;
typedef glm::u16vec2 TexCoord;
typedef uint32_t PackedNormal;

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
        Backend::draw_fullscreen_quad();
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

// for use with GL_UNSIGNED_INT_2_10_10_10_REV
inline PackedNormal normal_pack(const glm::vec4& v) {
    return ((PackedNormal)(1023.0f*v.x) << 0) | 
        ((PackedNormal)(1023.0f*v.y) << 10) | 
        ((PackedNormal)(1023.0f*v.z) << 20) | 
        ((PackedNormal)(3.0f*v.a) << 30);
}

struct SurfaceTriangles {
    SurfaceTriangles(void) { 
        wipe_memory(this, sizeof(SurfaceTriangles)); 
    }
    SurfaceTriangles(const uint32_t nv, const uint32_t nidx) {
        assert(nv && nidx);
        wipe_memory(this, sizeof(SurfaceTriangles));
        vertices = new glm::vec3[nv];
        texture_coordinates = new TexCoord[nv];
        normals = new PackedNormal[nv];
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
    TexCoord *texture_coordinates;

    // ditto for normals
    PackedNormal *normals;            // store as 2_10_10_10_REV, use normal_pack()

    // however, not true for indices
    uint32_t nIndices;
    GLushort *indices;
};

static const int NUM_TRIANGLES = 250;
SurfaceTriangles st(3*NUM_TRIANGLES, 3*NUM_TRIANGLES);

struct VertexAttributePointers {
    glm::vec3 *vertBuf;
    RGBA *colBuf;
    TexCoord *texCoordBuf;
    GLuint *normalBuf;
};

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

    explicit GeometryBuffer(void) : mIsGeometryBufferOpen(false), cmdBuf(nullptr), current_index(0) {
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
            close_geometry_buffer();

        glDeleteBuffers(NUM_GEOM_BUF, buf_id);
        glDeleteVertexArrays(1, &vao);
    }

    // opens the entire buffer (this is wasteful)
    void open_geometry_buffer(void) {
        if (IsOpen()) {
            LOG(LOG_WARNING, "Redundant GeometryBuffer opening");
            return;
        }
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_POSITION]);
        m_vertex_attr.vertBuf = (glm::vec3*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_COLOR]);
        m_vertex_attr.colBuf = (RGBA*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_TEXCOORD]);
        m_vertex_attr.texCoordBuf = (TexCoord*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_NORMAL]);
        m_vertex_attr.normalBuf = (GLuint*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        idxBuf = (GLushort*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        if (!(m_vertex_attr.vertBuf && m_vertex_attr.colBuf && m_vertex_attr.texCoordBuf))
            LOG(LOG_WARNING, "glMapBuffer(GL_ARRAY_BUFFER,) returned null");
        if (NULL == idxBuf)
            LOG(LOG_WARNING, "glMapBuffer(GL_ELEMENT_ARRAY_BUFFER,) returned null");

        mIsGeometryBufferOpen = true;
    }
    void close_geometry_buffer(void) {
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

        wipe_memory(&m_vertex_attr, sizeof(m_vertex_attr));
        idxBuf = nullptr;
        mIsGeometryBufferOpen = false;
    }
    void open_command_queue(void) {
        if (nullptr != cmdBuf) {
            LOG(LOG_WARNING, "Redundant CommandQueue opening");
            return;
        }

        cmdBuf = (DrawElementsIndirectCommand*)glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, cmd_queues.get_base_offset()*sizeof(DrawElementsIndirectCommand), cmd_queues.PARTITION_SIZE*sizeof(DrawElementsIndirectCommand), GL_MAP_WRITE_BIT);
        if (NULL == cmdBuf)
            LOG(LOG_WARNING, "glMapBuffer(GL_DRAW_INDIRECT_BUFFER,) returned null");
    }
    void close_command_queue(void) {
        if (nullptr == cmdBuf) {
            LOG(LOG_WARNING, "Redundant CommandQueue closing");
            return;
        }

        if (GL_TRUE != glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER))
            LOG(LOG_WARNING, "glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) failed");
        cmdBuf = nullptr;
    }
    inline bool IsOpen(void) const { return mIsGeometryBufferOpen; }

    // caller's responsibility to keep track of the location of the surface
    uint32_t add_surface(const SurfaceTriangles& st) {
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
            tex[i] = (st.texture_coordinates != nullptr) ? st.texture_coordinates[i] : TexCoord(0, 0);
            normals[i] = (st.normals != nullptr) ? st.normals[i] : normal_pack(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
        }

        for (i = 0;i < st.nIndices;++i)
            idx[i] = st.indices[i];

        current_index  += st.nIndices;
        return current_index - st.nIndices;
    }
    void add_draw_command(uint32_t offset, uint32_t nIndices) {
        assert (nullptr != cmdBuf);

        auto& cmd = *get_command_buffer_ptr();

        cmd.firstIndex = offset;
        cmd.idxCount = nIndices;
        cmd.baseVertex = 0;
        cmd.baseInstance = 0;
        cmd.instanceCount = 1;

        cmd_queues.Push();
    }

    GLuint get_vao(void) const { return vao; }
    GLuint get_indirect_buffer_id(void) const { return buf_id[INDIRECT_DRAW_CMD]; };

    glm::vec3 *get_vertex_buffer_pointer(void) const { return m_vertex_attr.vertBuf; }
    RGBA *get_color_buffer_ptr(void) const { return m_vertex_attr.colBuf; }
    TexCoord *get_texture_coordinate_buffer_pointer(void) const { return m_vertex_attr.texCoordBuf; }
    PackedNormal *get_normal_buffer_ptr(void) const { return m_vertex_attr.normalBuf; }

    GLushort *get_index_buffer_ptr(void) const { return idxBuf; }
    DrawElementsIndirectCommand *get_command_buffer_ptr(void) const { return cmdBuf; }

    // only manages offsets into buf_id[INDIRECT_DRAW_CMD]
    struct CommandQueues {
        static const uint8_t NUM_PARTITIONS = 2;
        static const uint16_t OFFSET = WORLD_NUM_INDIRECT_CMDS / NUM_PARTITIONS;
        static const uint16_t PARTITION_SIZE = OFFSET;
        explicit CommandQueues(void) {
            current_queue = 0;
            for (int i = 0;i < NUM_PARTITIONS;++i) {
                top[i] = 0;
                base[i] = i*PARTITION_SIZE;
            }
        }
        void Push(void) {
            if (top[current_queue] == PARTITION_SIZE) {
                LOG(LOG_WARNING, "CommandQueue topped out at %u", top);
                return;
            }
            ++top[current_queue];
        }
        void Pop(void) {
            if (top == 0) {
                LOG(LOG_WARNING, "CommandQueue underflow");
                return;
            }
            --top[current_queue];
        }
        uint16_t get_base_offset(void) const { return base[current_queue]; }
        uint32_t get_base_offset_in_bytes(void) const { return get_base_offset()*sizeof(DrawElementsIndirectCommand); }
        void Swap(void) { current_queue = (current_queue+1)%NUM_PARTITIONS; }
        void Clear(void) { top[current_queue] = 0; }
        uint16_t get_size(void) const { return top[current_queue]; }
        uint16_t get_top(void) const { return get_base_offset() + top[current_queue]; };
        uint16_t top[NUM_PARTITIONS];
        uint16_t base[NUM_PARTITIONS];
        uint8_t current_queue;
    };

    CommandQueues cmd_queues;

private:
    GLuint buf_id[NUM_GEOM_BUF];
    GLuint vao;

    VertexAttributePointers m_vertex_attr;

    GLushort *idxBuf;
    DrawElementsIndirectCommand *cmdBuf;

    uint32_t current_index;

    bool mIsGeometryBufferOpen;
};

struct Backend::Impl {
    inline void clear_performance_counters(void) { wipe_memory(&mPerfCounts, sizeof(PerfCounters)); }
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

        mSpecs.extensions.resize(mSpecs.nExtensions, "<null>");

        for (int i = 0;i < mSpecs.nExtensions;++i) {
            static const GLubyte *c;
            c = glGetStringi(GL_EXTENSIONS, i);
            LOG(LOG_TRACE, "extension: %s", c);
            mSpecs.extensions[i] = (const char*)c;
        }

    }
    GeometryBuffer geom_buf;

    bool startup(int w, int h) {
        // misc. defaults
        current_screen_width = w;
        current_screen_height = h;
        screenshotCounter = 0;
        offscreenRender = PIXELATED;

        clear_performance_counters();
        query_hardware_specs();

        // Allocate world geometry buffers

        return true;
    }
    void shutdown(void) {
        // glDelete* calls should go here
    }

    void begin_frame(void) {
        clear_performance_counters();
        if (offscreenRender) {
            if (nullptr == offscreenFB)
                offscreenFB = new Framebuffer(OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
            offscreenFB->Bind();    
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        geom_buf.cmd_queues.Clear();
        geom_buf.open_command_queue();
    }

    uint32_t loc;

    void add_tris(void) {
        static bool firstTime = true;
        if (false == firstTime)
            return;
        firstTime = false;
        GLushort i;
        for (i = 0;i < 3*NUM_TRIANGLES;i += 3) {
            glm::vec3 v0(uniform(-1.0f, 1.0f), uniform(-1.0f, 1.0f), uniform(-1.0f, 1.0f));
            glm::vec3 v1(v0 + glm::vec3(uniform(0.0f, 0.5f), uniform(0.0f, 0.5f), 0.0f));
            glm::vec3 v2(v0 + glm::vec3(uniform(0.0f, 0.5f), uniform(0.0f, 0.5f), 0.0f));
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
    void end_frame(void) {
        mImpl->geom_buf.add_draw_command(loc, st.nIndices);
        geom_buf.close_command_queue();
        glBindVertexArray(geom_buf.get_vao());
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, (void*)(geom_buf.cmd_queues.get_base_offset_in_bytes()), geom_buf.cmd_queues.get_size(), 0);
        if (offscreenRender)
            offscreenFB->Blit(current_screen_width, current_screen_height);
        geom_buf.cmd_queues.Swap();
    }
    void resize(int w, int h) {
        glViewport(0, 0, w, h);
        current_screen_width = w;
        current_screen_height = h;
    }
    void screenshot(void) {
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

    void disable_blending(void) { glDisable(GL_BLEND); }
    void draw_fullscreen_quad(void) {
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

    void enable_additive_blending(void) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }
    void enable_blending(void) {
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
void Backend::screenshot(void) { mImpl->screenshot(); }
void Backend::draw_fullscreen_quad(void) { mImpl->draw_fullscreen_quad(); }
void Backend::add_tris(void) { mImpl->add_tris(); }
