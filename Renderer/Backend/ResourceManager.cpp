// Copyright [year] <Copyright Owner>
// Dependencies: Backend, PhysFS, Lua
#include "ResourceManager.h"
#include "../../LuaBindings.h"
#include "Backend.h"
#include <physfs/physfs.h>
#include <lua/lua.hpp>

static const char *defaultSearchPaths[] = {
    "data.zip",
    "data",
};

struct ResourceManager::Impl {
    bool startup(void) {
    }
    void shutdown(void) {
    }
};
ResourceManager::ResourceManager(void) {
    m_impl = std::unique_ptr<Impl>(new Impl());

}
ResourceManager::~ResourceManager(void) {
    m_impl.reset(nullptr);
}

bool ResourceManager::startup(void) {
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

void ResourceManager::shutdown(void) {
    PHYSFS_deinit();
}

bool startup() {
    return false;
}

void shutdown(void) {
    PHYSFS_deinit();
}