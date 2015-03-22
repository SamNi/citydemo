// Copyright [year] <Copyright Owner>
// Dependencies: Backend, PhysFS, Lua
#include "../../LuaBindings.h"
#include "Backend_local.h"
#include <physfs/physfs.h>
#include <lua/lua.hpp>
#include <unordered_map>

static const char *defaultSearchPaths[] = {
    "data.zip",
    "data",
};

struct ResourceManager::Impl {
    resource_handle                                 m_counter;
    std::unordered_map<resource_handle, Resource>   m_resources;
    bool startup(void) {
        m_counter = 1;

        int i;
        if (!PHYSFS_init("default")) {
            LOG(LOG_CRITICAL, "PHYSFS_init failed");
            return false;
        }
        for (i = 0;i < sizeof(defaultSearchPaths) / sizeof(const char *);++i) {
            if (!PHYSFS_mount(defaultSearchPaths[i], "/", 0))
                LOG(LOG_WARNING, "PHYSFS_mount failed to open %s: %s\n", defaultSearchPaths[i], PHYSFS_getLastError());
        }
        PHYSFS_file *fin;
        PHYSFS_sint64 fileLen;
        char *buf;

        fin = PHYSFS_openRead("autoexec.lua");
        if (nullptr == fin) {
            LOG(LOG_CRITICAL, "autoexec.lua is missing THIS IS REQUIRED\n");
            return false;
        }
    
        fileLen = PHYSFS_fileLength(fin);
        buf = new char[fileLen+1];
        PHYSFS_read(fin, buf, 1, fileLen);
        buf[fileLen] = '\0';
        if (false == Lua::exec(buf)) {
            LOG(LOG_CRITICAL, "%s\n", lua_tostring(Lua::get_state(),-1));
            delete[] buf;
            return false;
        }
        LOG(LOG_VERBOSE, "autoexec.lua executed successfully\n");
        delete[] buf;
        return true;
    }
    void shutdown(void) {
        PHYSFS_deinit();
    }

    // Framebuffers
    resource_handle create_framebuffer(uint16_t w, uint16_t h) {
        return 0;
    }
    void bind_framebuffer(resource_handle h) {

    }
    void unbind_framebuffer(void) {

    }
    void blit_framebuffer(resource_handle h) {

    }

    explicit Impl(void) : m_counter(1) {}
    ~Impl(void) { }
};

ResourceManager::ResourceManager(void) : m_impl(std::unique_ptr<Impl>(new Impl())) { }
ResourceManager::~ResourceManager(void) { m_impl.reset(nullptr); }
bool ResourceManager::startup(void) { return m_impl->startup();  }
void ResourceManager::shutdown(void) { m_impl->shutdown(); }

resource_handle ResourceManager::create_framebuffer(uint16_t w, uint16_t h) { return m_impl->create_framebuffer(w, h); }
void ResourceManager::bind_framebuffer(resource_handle h) { m_impl->bind_framebuffer(h); }
void ResourceManager::unbind_framebuffer(void) { m_impl->unbind_framebuffer(); }
void ResourceManager::blit_framebuffer(resource_handle h) { m_impl->blit_framebuffer(h); }