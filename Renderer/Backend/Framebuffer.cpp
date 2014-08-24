#include "Backend_local.h"

struct Framebuffer::Impl {
    explicit Impl(uint16_t w, uint16_t h) {
        width = w;
        height = h;
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
    ~Impl(void) {
        glDeleteFramebuffers(1, &framebufferID);
    }
    void bind(void) const {
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
        glBindTexture(GL_TEXTURE_2D, oldTextureID);
        glViewport(0, 0, width, height);
        checkGL();
    }

    void blit(int w, int h) const {
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
    uint32_t          framebufferID;
    uint32_t          textureID;
    int32_t           oldTextureID;
    uint16_t          width, height;
};

Framebuffer::Framebuffer(uint16_t w, uint16_t h) {
    m_impl = std::unique_ptr<Impl>(new Impl(w, h));
}
Framebuffer::~Framebuffer(void) {
    m_impl.reset(nullptr);
}
void Framebuffer::bind(void) const {
    m_impl->bind();
}
void Framebuffer::blit(int w, int h) const {
    m_impl->blit(w, h);
}