#include "FBO.h"

static const int            rtWidth = 8;
static const int            rtHeight = 8;

static const GLfloat quad[] = {
    -1.0f, -1.0f,
    1.0f, -1.0f,
    1.0f, 1.0f,
    -1.0f, 1.0f,
};

static const GLfloat texCoords[] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,
};

FBO::FBO(void) {
    // Vertex buffers
    /*
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 2*4*sizeof(GL_FLOAT), quad, GL_STATIC_DRAW);
    checkGL();

    glGenBuffers(1, &vbo_tex);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_tex);
    glBufferData(GL_ARRAY_BUFFER, 2*4*sizeof(GL_FLOAT), texCoords, GL_STATIC_DRAW);
    checkGL();


    // Vertex array
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    checkGL();
*/
    // Texture
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &renderTargetId);
    glBindTexture(GL_TEXTURE_2D, renderTargetId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_RGBA,
        rtWidth,
        rtHeight,
        0,
        GL_RGB, GL_UNSIGNED_BYTE,
        nullptr);
    checkGL();

    // Framebuffer
    glGenFramebuffers(1, &fboId);
    glBindFramebuffer(GL_FRAMEBUFFER, fboId);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderTargetId, 0);
    checkGL();

    GLenum err = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch (err) {
    case GL_FRAMEBUFFER_COMPLETE:
        fprintf(stderr, "Ok!\n");
        break;
    case GL_FRAMEBUFFER_UNDEFINED:
        fprintf(stderr, "GL_FRAMEBUFFER_UNDEFINED");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT :
        fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT ");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT :
        fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT ");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER :
        fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER ");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER :
        fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER ");
        break;
    case GL_FRAMEBUFFER_UNSUPPORTED :
        fprintf(stderr, "GL_FRAMEBUFFER_UNSUPPORTED ");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE  :
        fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE  ");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS  :
        fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS  ");
        break;
    case 0:
        fprintf(stderr, "Zero\n");
        break;
    case GL_INVALID_ENUM:
        fprintf(stderr, "Enum\n");
        break;
    default:
        fprintf(stderr, "Other\n");
    }
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);


    /*
    glGenTextures(1, &renderTargetId);
    glBindTexture(GL_TEXTURE_2D, renderTargetId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    checkGL();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, rtWidth, rtHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    checkGL();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTargetId, 0);
    checkGL();

    checkGL();

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderTargetId, 0);
    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    checkGL();

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 2*4*sizeof(GLfloat), quad, GL_STATIC_DRAW);

    checkGL();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    checkGL();

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, fboId, GL_RENDERBUFFER, renderTargetId);
    checkGL();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);*/
}

void FBO::Bind(void) {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboId);
    glViewport(0, 0, rtWidth, rtHeight);
}

void FBO::Unbind(void) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glViewport(0, 0, 1600, 900);
}

void FBO::Blit() {
    /*
    Unbind();
    checkGL();
    glUseProgram(passthruId);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    checkGL();*/
}

FBO::~FBO(void) {
    glDeleteTextures(1, &renderTargetId);
    glDeleteFramebuffers(1, &fboId);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &vao);
}