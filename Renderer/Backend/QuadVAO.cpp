#include "Backend_local.h"

GLuint QuadVAO::m_vao_handle = 0;
ImmutableBufPtr QuadVAO::m_position_buffer = nullptr;
ImmutableBufPtr QuadVAO::m_color_buffer = nullptr;
ImmutableBufPtr QuadVAO::m_texcoord_buffer = nullptr;
ImmutableBufPtr QuadVAO::m_normal_buffer = nullptr;


GLuint QuadVAO::get_vao(void) { 
    // lazy initialization
    if (0 == m_vao_handle)
        _open();
    return m_vao_handle;
}
void QuadVAO::reset(void) {
    m_position_buffer.reset(nullptr);
    m_color_buffer.reset(nullptr);
    m_texcoord_buffer.reset(nullptr);
    m_normal_buffer.reset(nullptr);
    m_vao_handle = 0;
}

// please don't instantiate me
QuadVAO::QuadVAO(void) { }
QuadVAO::~QuadVAO(void) { }

void QuadVAO::_open(void) {
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