#include "Backend_local.h"

TriList::TriList(void) : vao(0), vbo_points(0), vbo_indices(0) { }
void TriList::Init(const glm::vec3* v, int n) {
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
    this->nVerts = n;
    this->nIdx = 0;
}
void TriList::Init(const glm::vec3 *v, const GLubyte *indices, int n, int nIndices) {
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
    glBufferData(GL_ARRAY_BUFFER, 3*n*sizeof(GLfloat), glm::value_ptr(*v), GL_STATIC_DRAW);

    glGenBuffers(1, &vbo_indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, nIndices*sizeof(GLubyte), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    idx = indices;

    indexed = true;
    this->nVerts = n;
    this->nIdx = nIndices;
}

void TriList::Draw(void) const {
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
GLuint TriList::get_vao(void) const { return vao; }
GLuint TriList::get_vbo(void) const { return vbo_points; }
GLuint TriList::get_vbo_indices(void) const { return vbo_indices; }
const GLubyte* TriList::get_indices(void) const { return idx; }
bool TriList::IsValid(void) const { return vao && vbo_points && vbo_indices && idx; }
