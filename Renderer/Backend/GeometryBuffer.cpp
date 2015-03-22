#include "Backend_local.h"


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

GeometryBuffer::GeometryBuffer(void) : mIsGeometryBufferOpen(false), cmdBuf(nullptr), current_index(0) {
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
GeometryBuffer::~GeometryBuffer(void) {
    if (IsOpen())
        close_geometry_buffer();

    glDeleteBuffers(NUM_GEOM_BUF, buf_id);
    glDeleteVertexArrays(1, &vao);
}

// opens the entire buffer (this is wasteful)
void GeometryBuffer::open_geometry_buffer(void) {
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
void GeometryBuffer::close_geometry_buffer(void) {
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
void GeometryBuffer::open_command_queue(void) {
    if (nullptr != cmdBuf) {
        LOG(LOG_WARNING, "Redundant CommandQueue opening");
        return;
    }

    cmdBuf = static_cast<DrawElementsIndirectCommand*>(glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, cmd_queues.get_base_offset()*sizeof(DrawElementsIndirectCommand), cmd_queues.PARTITION_SIZE*sizeof(DrawElementsIndirectCommand), GL_MAP_WRITE_BIT));
    if (NULL == cmdBuf)
        LOG(LOG_WARNING, "glMapBuffer(GL_DRAW_INDIRECT_BUFFER,) returned null");
}
void GeometryBuffer::close_command_queue(void) {
    if (nullptr == cmdBuf) {
        LOG(LOG_WARNING, "Redundant CommandQueue closing");
        return;
    }

    if (GL_TRUE != glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER))
        LOG(LOG_WARNING, "glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) failed");
    cmdBuf = nullptr;
}
bool GeometryBuffer::IsOpen(void) const { return mIsGeometryBufferOpen; }

// caller's responsibility to keep track of the location of the surface
uint32_t GeometryBuffer::add_surface(const SurfaceTriangles& st) {
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
void GeometryBuffer::add_draw_command(uint32_t offset, uint32_t nIndices) {
    assert (nullptr != cmdBuf);

    auto& cmd = *get_command_buffer_ptr();

    cmd.firstIndex = offset;
    cmd.idxCount = nIndices;
    cmd.baseVertex = 0;
    cmd.baseInstance = 0;
    cmd.instanceCount = 1;

    cmd_queues.Push();
}

GLuint GeometryBuffer::get_vao(void) const { return vao; }
GLuint GeometryBuffer::get_indirect_buffer_id(void) const { return buf_id[INDIRECT_DRAW_CMD]; };
const VertexAttributePointers& GeometryBuffer::get_vert_attr(void) const { return m_vertex_attr; }

GLuint *GeometryBuffer::get_index_buffer_ptr(void) const { return idxBuf; }
DrawElementsIndirectCommand *GeometryBuffer::get_command_buffer_ptr(void) const { return cmdBuf; }

// only manages offsets into buf_id[INDIRECT_DRAW_CMD]
CommandQueues::CommandQueues(void) {
    current_queue = 0;
    for (int i = 0;i < NUM_PARTITIONS;++i) {
        top[i] = 0;
        base[i] = (uint16_t)i*PARTITION_SIZE;
    }
}
void CommandQueues::Push(void) {
    if (top[current_queue] == PARTITION_SIZE) {
        LOG(LOG_WARNING, "CommandQueue topped out at %u", top);
        return;
    }
    ++top[current_queue];
}
void CommandQueues::Pop(void) {
    if (top == 0) {
        LOG(LOG_WARNING, "CommandQueue underflow");
        return;
    }
    --top[current_queue];
}
uint16_t CommandQueues::get_base_offset(void) const { return base[current_queue]; }
uint32_t CommandQueues::get_base_offset_in_bytes(void) const { return get_base_offset()*sizeof(DrawElementsIndirectCommand); }
void CommandQueues::Swap(void) { current_queue = (current_queue+1)%NUM_PARTITIONS; }
void CommandQueues::Clear(void) { top[current_queue] = 0; }
uint16_t CommandQueues::get_size(void) const { return top[current_queue]; }
uint16_t CommandQueues::get_top(void) const { return get_base_offset() + top[current_queue]; };