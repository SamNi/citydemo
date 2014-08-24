#include "Backend_local.h"

OpenGLBufferImmutable::OpenGLBufferImmutable(GLenum target, GLenum flags, uint32_t num_bytes) : m_num_bytes(num_bytes), m_flags(flags), m_target(target) {
    _open(target, flags, num_bytes, nullptr);
}
OpenGLBufferImmutable::OpenGLBufferImmutable(GLenum target, GLenum flags, uint32_t num_bytes, const void *data) : m_num_bytes(num_bytes), m_flags(flags), m_target(target) {
    _open(target, flags, num_bytes, data);
}
OpenGLBufferImmutable::~OpenGLBufferImmutable(void) {
    glDeleteBuffers(1, &m_buffer_handle);
    checkGL();
}
GLuint OpenGLBufferImmutable::get_handle(void) const { 
    return m_buffer_handle; 
};
void OpenGLBufferImmutable::bind(void) const { 
    glBindBuffer(m_target, m_buffer_handle); 
    checkGL();
}

void OpenGLBufferImmutable::_open(GLenum target, GLenum flags, uint32_t num_bytes, const void *data) {
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

