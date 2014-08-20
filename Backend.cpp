// Copyright [year] <Copyright Owner>
// Goal: isolate the direct OpenGL calls, enums to the back end
#include "./Backend.h"
#include "./GL.h"
#include "./GLM.h"
#include "Shader.h"
#include "Camera.h"
#include <png.h>            // for writing screenshots

#pragma warning(disable : 4800)

static const int        OFFSCREEN_WIDTH =           512;
static const int        OFFSCREEN_HEIGHT =          512;
static const bool       PIXELATED =                 false;

struct OpenGLBufferImmutable;

typedef std::unique_ptr<OpenGLBufferImmutable> ImmutableBufPtr;

inline void wipe_memory(void *dest, size_t n) { memset(dest, NULL, n); }

// paper thin wrappers with debug-only sanity checks
struct OpenGLBufferImmutable {
    OpenGLBufferImmutable(GLenum target, GLenum flags, uint32_t num_bytes) : m_num_bytes(num_bytes), m_flags(flags), m_target(target) {
        _open(target, flags, num_bytes, nullptr);
    }
    OpenGLBufferImmutable(GLenum target, GLenum flags, uint32_t num_bytes, const void *data) : m_num_bytes(num_bytes), m_flags(flags), m_target(target) {
        _open(target, flags, num_bytes, data);
    }
    ~OpenGLBufferImmutable(void) {
        glDeleteBuffers(1, &m_buffer_handle);
        checkGL();
    }
    GLuint get_handle(void) const { 
        return m_buffer_handle; 
    };
    void bind(void) const { 
        glBindBuffer(m_target, m_buffer_handle); 
        checkGL();
    }
private:
    void _open(GLenum target, GLenum flags, uint32_t num_bytes, const void *data) {
        glGenBuffers(1, &m_buffer_handle);
        glBindBuffer(target, m_buffer_handle);
        glBufferStorage(target, num_bytes, data, flags);
        checkGL();
        if (nullptr != data) {
            auto p = glMapBuffer(target, GL_WRITE_ONLY);
            assert(nullptr != p);
            checkGL();
            memcpy(p, data, num_bytes);
            glUnmapBuffer(target);
        } else 
            glClearBufferData(target, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glBindBuffer(target, 0);
        checkGL();
    }
    GLenum          m_flags;
    GLenum          m_target;
    GLuint          m_buffer_handle;
    uint32_t        m_num_bytes;
};

struct QuadVAO {
    static GLuint get_vao(void) { 
        // lazy initialization
        if (0 == m_vao_handle)
            _open();
        return m_vao_handle;
    }
    static void reset(void) {
        m_position_buffer.reset(nullptr);
        m_color_buffer.reset(nullptr);
        m_texcoord_buffer.reset(nullptr);
        m_normal_buffer.reset(nullptr);
        m_vao_handle = 0;
    }
private:
    // please don't instantiate me
    explicit QuadVAO(void) { }
    ~QuadVAO(void) { }
    static const GLenum TARGET = GL_ARRAY_BUFFER;
    static const GLenum FLAGS = GL_MAP_WRITE_BIT;
    static void _open(void) {
        static const glm::vec3 points[] = {
            glm::vec3(-1.0f, -1.0f, 0.0f),
            glm::vec3(+1.0f, -1.0f, 0.0f),
            glm::vec3(+1.0f, +1.0f, 0.0f),
            glm::vec3(-1.0f, +1.0f, 0.0f),
        };
        static const RGBAPixel colors[] = {
            RGBAPixel(255, 255, 255, 255),
            RGBAPixel(255, 255, 255, 255),
            RGBAPixel(255, 255, 255, 255),
            RGBAPixel(255, 255, 255, 255),
        };
        static const TexCoord texCoords[] = {
            TexCoord(0, 0),
            TexCoord(65535, 0),
            TexCoord(65535, 65535),
            TexCoord(0, 65535),
        };

        static const glm::vec3 normals[] = {
            glm::vec3(-1.0f, +1.0f, -1.0f),
            glm::vec3(-1.0f, -1.0f, -1.0f),
            glm::vec3(+1.0f, -1.0f, -1.0f),
            glm::vec3(+1.0f, +1.0f, -1.0f),
        };

        if (0 != m_vao_handle)
            return;

        glGenVertexArrays(1, &m_vao_handle);
        glBindVertexArray(m_vao_handle);

        m_position_buffer = ImmutableBufPtr(new OpenGLBufferImmutable(TARGET, FLAGS, sizeof(points), points));
        m_color_buffer = ImmutableBufPtr(new OpenGLBufferImmutable(TARGET, FLAGS, sizeof(colors), colors));
        m_texcoord_buffer = ImmutableBufPtr(new OpenGLBufferImmutable(TARGET, FLAGS, sizeof(texCoords), texCoords));
        m_normal_buffer = ImmutableBufPtr(new OpenGLBufferImmutable(TARGET, FLAGS, sizeof(normals), normals));
        m_position_buffer->bind();
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        m_color_buffer->bind();
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
        m_texcoord_buffer->bind();
        glVertexAttribPointer(2, 2, GL_UNSIGNED_SHORT, GL_TRUE, 0, 0);
        m_normal_buffer->bind();
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        checkGL();
        glBindVertexArray(0);
    }

    static GLuint m_vao_handle;
    static ImmutableBufPtr m_position_buffer;
    static ImmutableBufPtr m_color_buffer;
    static ImmutableBufPtr m_texcoord_buffer;
    static ImmutableBufPtr m_normal_buffer;
};
GLuint QuadVAO::m_vao_handle = 0;
ImmutableBufPtr QuadVAO::m_position_buffer = nullptr;
ImmutableBufPtr QuadVAO::m_color_buffer = nullptr;
ImmutableBufPtr QuadVAO::m_texcoord_buffer = nullptr;
ImmutableBufPtr QuadVAO::m_normal_buffer = nullptr;

#include "GUI.h"

using namespace GUI;

struct MyWidget : public Widget {
    explicit MyWidget(void) {
        // 
    }

    virtual void draw(void) const {
        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, 5);
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
        static const auto identity_matrix = glm::mat4x4(1.0f);

        // switch to the visible render buffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glViewport(0, 0, w, h);
        Backend::set_modelview(identity_matrix);
        Backend::set_projection(identity_matrix);
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

SurfaceTriangles::SurfaceTriangles(void) { 
    wipe_memory(this, sizeof(SurfaceTriangles)); 
}
SurfaceTriangles::SurfaceTriangles(const uint32_t nv, const uint32_t nidx) {
    assert(nv && nidx);
    wipe_memory(this, sizeof(SurfaceTriangles));
    vertices = new glm::vec3[nv];
    colors = new RGBAPixel[nv];
    texture_coordinates = new TexCoord[nv];
    normals = new PackedNormal[nv];
    indices = new GLuint[nidx];
    nVertices = nv;
    nIndices = nidx;
}
SurfaceTriangles::~SurfaceTriangles(void) {
    delete[] vertices;
    delete[] colors;
    delete[] texture_coordinates;
    delete[] normals;
    delete[] indices;
}
uint32_t SurfaceTriangles::GetSize(void) const {
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
        ret += sizeof(GLuint)*nIndices;
    return ret;
}

static const int NUM_TRIANGLES = 1000;
SurfaceTriangles st(3*NUM_TRIANGLES, 3*NUM_TRIANGLES);

struct VertexAttributePointers {
    glm::vec3 *vertBuf;
    RGBAPixel *colBuf;
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
    static const uint32_t WORLD_IDX_BUF_SIZE = WORLD_NUM_IDX*sizeof(GLuint); // a single ushort for the index
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
        m_vertex_attr.colBuf = (RGBAPixel*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_TEXCOORD]);
        m_vertex_attr.texCoordBuf = (TexCoord*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        glBindBuffer(GL_ARRAY_BUFFER, buf_id[VERTEX_ATTR_NORMAL]);
        m_vertex_attr.normalBuf = (GLuint*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        idxBuf = (GLuint*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
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
        auto vertex_attributes = get_vert_attr();
        auto idx = get_index_buffer_ptr();

        for (i = 0;i < st.nVertices;++i) {
            vertex_attributes.vertBuf[i] = st.vertices[i];
            vertex_attributes.colBuf[i] = (st.colors != nullptr) ? st.colors[i] : RGBAPixel(255,255,255,255);
            vertex_attributes.texCoordBuf[i] = (st.texture_coordinates != nullptr) ? st.texture_coordinates[i] : TexCoord(0, 0);
            vertex_attributes.normalBuf[i] = (st.normals != nullptr) ? st.normals[i] : normal_pack(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
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
    const VertexAttributePointers& get_vert_attr(void) const { return m_vertex_attr; }

    GLuint *get_index_buffer_ptr(void) const { return idxBuf; }
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

    GLuint *idxBuf;
    DrawElementsIndirectCommand *cmdBuf;

    uint32_t current_index;

    bool mIsGeometryBufferOpen;
};

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
        //geom_buf.open_command_queue();
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
        //mImpl->geom_buf.add_draw_command(loc, st.nIndices);
        //geom_buf.close_command_queue();
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
            offscreenFB->Blit(current_screen_width, current_screen_height);
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
