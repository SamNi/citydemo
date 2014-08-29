#ifndef _BACKEND_LOCAL_H
#define _BACKEND_LOCAL_H
#include "Backend.h"
#include "GL.h"

inline void wipe_memory(void *dest, size_t n) { memset(dest, NULL, n); }

struct VertexAttributePointers {
    glm::vec3 *vertBuf;
    RGBAPixel *colBuf;
    TexCoord *texCoordBuf;
    GLuint *normalBuf;
};

// OpenGLBufferImmutable.cpp
struct OpenGLBufferImmutable {
    OpenGLBufferImmutable(GLenum target, GLenum flags, uint32_t num_bytes);
    OpenGLBufferImmutable(GLenum target, GLenum flags, uint32_t num_bytes, const void *data);
    ~OpenGLBufferImmutable(void);
    GLuint get_handle(void) const;
    void bind(void) const;

private:
    void _open(GLenum target, GLenum flags, uint32_t num_bytes, const void *data);
    GLenum          m_flags;
    GLenum          m_target;
    GLuint          m_buffer_handle;
    uint32_t        m_num_bytes;
};

typedef std::unique_ptr<OpenGLBufferImmutable> ImmutableBufPtr;

// QuadVAO.cpp
struct QuadVAO {
    static GLuint get_vao(void);
    static void reset(void);
    static void draw(void);

private:
    // please don't instantiate me
    explicit QuadVAO(void);
    ~QuadVAO(void);
    static const GLenum TARGET = GL_ARRAY_BUFFER;
    static const GLenum FLAGS = GL_MAP_WRITE_BIT;
    static void _open(void);

    static GLuint m_vao_handle;
    static ImmutableBufPtr m_position_buffer;
    static ImmutableBufPtr m_color_buffer;
    static ImmutableBufPtr m_texcoord_buffer;
    static ImmutableBufPtr m_normal_buffer;
};

// Texture.cpp
struct TextureManager {
    static bool                     startup(void);
    static void                     shutdown(void);
    static uint32_t                 load(const char *path);
    static void                     bind(uint32_t textureID);
    static void                     remove(uint32_t textureID);
private:
    struct Impl;
    typedef std::unique_ptr<Impl> ImplPtr;
    static ImplPtr m_impl;
};

struct FramebufferManager {
    explicit FramebufferManager(void);
    ~FramebufferManager(void);
    uint32_t        create(uint16_t w, uint16_t h);
    void            bind(uint32_t handle);
    void            unbind(void);
    void            blit(uint32_t handle);

private:
    struct Impl;
    typedef std::unique_ptr<Impl> ImplPtr;
    ImplPtr m_impl;
};

struct DrawElementsIndirectCommand {
    unsigned int idxCount, instanceCount, firstIndex, baseVertex, baseInstance;
};

// TriList.cpp
struct TriList {
    explicit TriList(void);
    void Init(const glm::vec3* v, int n);
    void Init(const glm::vec3 *v, const GLubyte *indices, int nVerts, int nIndices);
    void Draw(void) const;
    GLuint  get_vao(void) const;
    GLuint  get_vbo(void) const;
    GLuint get_vbo_indices(void) const;
    const GLubyte* get_indices(void) const;
    bool IsValid(void) const;

private:
    GLuint indirect_draw_buffer;
    GLuint vbo_points, vbo_indices, vao;
    const GLubyte *idx;
    int nVerts, nIdx;
    bool indexed;
};

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
static const uint32_t WORLD_IDX_BUF_SIZE = WORLD_NUM_IDX*sizeof(GLuint); // a single ushort for the index
static const uint32_t WORLD_INDIRECT_CMD_BUF_SIZE = WORLD_NUM_INDIRECT_CMDS*sizeof(DrawElementsIndirectCommand);

static const GLenum GEOMETRY_BUF_STORAGE_FLAGS = GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT;

// only manages offsets into buf_id[INDIRECT_DRAW_CMD]
struct CommandQueues {
    static const uint8_t NUM_PARTITIONS = 2;
    static const uint16_t OFFSET = WORLD_NUM_INDIRECT_CMDS / NUM_PARTITIONS;
    static const uint16_t PARTITION_SIZE = OFFSET;

    explicit CommandQueues(void);
    void Push(void);
    void Pop(void);
    uint16_t get_base_offset(void) const;
    uint32_t get_base_offset_in_bytes(void) const;
    void Swap(void);
    void Clear(void);
    uint16_t get_size(void) const;
    uint16_t get_top(void) const;
    uint16_t top[NUM_PARTITIONS];
    uint16_t base[NUM_PARTITIONS];
    uint8_t current_queue;
};


struct GeometryBuffer {
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

    explicit GeometryBuffer(void);
    ~GeometryBuffer(void);

    void open_geometry_buffer(void);
    void close_geometry_buffer(void);
    void open_command_queue(void);
    void close_command_queue(void);
    bool IsOpen(void) const;

    // caller's responsibility to keep track of the location of the surface
    uint32_t add_surface(const SurfaceTriangles& st);
    void add_draw_command(uint32_t offset, uint32_t nIndices);

    GLuint get_vao(void) const;
    GLuint get_indirect_buffer_id(void) const;
    const VertexAttributePointers& get_vert_attr(void) const;

    GLuint *get_index_buffer_ptr(void) const;
    DrawElementsIndirectCommand *get_command_buffer_ptr(void) const;

    CommandQueues cmd_queues;

private:
    GLuint buf_id[NUM_GEOM_BUF];
    GLuint vao;
    VertexAttributePointers m_vertex_attr;
    GLuint *idxBuf;
    DrawElementsIndirectCommand *cmdBuf;
    uint32_t current_index;
    bool mIsGeometryBufferOpen;
};

// GBuffer.cpp
struct GBuffer {
    explicit GBuffer(uint16_t w, uint16_t h);
    ~GBuffer(void);
    void bind(void);
private:
    struct Impl;
    typedef std::unique_ptr<Impl> ImplPtr;
    ImplPtr m_impl;
};

#endif  // ~_BACKEND_LOCAL_H