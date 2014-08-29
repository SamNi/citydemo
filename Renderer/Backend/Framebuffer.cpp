#include "Backend_local.h"
#include <unordered_map>

struct Framebuffer {
    explicit Framebuffer(uint16_t w, uint16_t h);
    ~Framebuffer(void);
    void bind(void) const;
    void blit(void) const;
    uint32_t get_texture_id(void) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

struct Framebuffer::Impl {
    explicit Impl(uint16_t w, uint16_t h) : width(w), height(h) {
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

    void blit() const {
        static const auto identity_matrix = glm::mat4x4(1.0f);

        // switch to the visible render buffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, textureID); // the texture we just drew to is now the source
        Backend::reset_viewport();
        Backend::set_modelview(identity_matrix);
        Backend::set_projection(identity_matrix);
        Backend::disable_depth_testing();
        Backend::draw_fullscreen_quad();
    }
    uint32_t          framebufferID;
    uint32_t          textureID;
    int32_t           oldTextureID;
    uint16_t          width, height;
};

Framebuffer::Framebuffer(uint16_t w, uint16_t h) : m_impl(std::unique_ptr<Impl>(new Impl(w,h))) { }
Framebuffer::~Framebuffer(void) { m_impl.reset(nullptr); }
void Framebuffer::bind(void) const { m_impl->bind(); }
void Framebuffer::blit(void) const { m_impl->blit(); }
uint32_t Framebuffer::get_texture_id(void) const { return m_impl->textureID; }


// -------------------------------------------------------
// FramebufferManager
// -------------------------------------------------------

struct FramebufferManager::Impl {
    static uint32_t m_fb_counter;

    typedef std::unique_ptr<Framebuffer> FramebufferPtr;
    std::unordered_map<uint32_t, FramebufferPtr> m_framebuffer_collection;

    uint32_t create(uint16_t w, uint16_t h) {
        assert((w > 0) && (h > 0));
        m_framebuffer_collection.emplace(m_fb_counter, FramebufferPtr(new Framebuffer(w, h)));
        return m_fb_counter++;
    }
    void bind(uint32_t handle) {
        auto it = m_framebuffer_collection.find(handle);
        if (it == m_framebuffer_collection.end()) {
            LOG(LOG_WARNING, "FramebufferManager::bind called on nonexistent handle %u", handle);
            return;
        }
        it->second->bind();
    }
    void unbind(void) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    void blit(uint32_t handle) {
        auto it = m_framebuffer_collection.find(handle);
        if (it == m_framebuffer_collection.end()) {
            LOG(LOG_WARNING, "FramebufferManager::blit called on nonexistent handle %u", handle);
            return;
        }
        it->second->blit();
    }
};
uint32_t FramebufferManager::Impl::m_fb_counter = 1;

FramebufferManager::FramebufferManager(void) : m_impl(ImplPtr(new Impl())) { }
FramebufferManager::~FramebufferManager(void) { m_impl.reset(nullptr); }
uint32_t FramebufferManager::create(uint16_t w, uint16_t h) { return m_impl->create(w, h); }
void FramebufferManager::bind(uint32_t handle) { m_impl->bind(handle); }
void FramebufferManager::unbind(void) { m_impl->unbind(); }
void FramebufferManager::blit(uint32_t handle) { m_impl->blit(handle); }