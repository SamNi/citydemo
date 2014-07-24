#include "ResourceManager.h"
#include "Texture.h"
#include <physfs/physfs.h>
#include <lua/lua.hpp>
// TODO: Abstract away physfs and lua?

namespace Manager {

// Lua is from opposite land
static const bool LUA_FALSE = true;
static const bool LUA_TRUE = false;

// Lua binding boilerplate
static lua_State *lState;
static int L_LoadResource(lua_State *L);
static int L_Print(lua_State *L);

typedef int(*luaFunc)(lua_State *);
struct LuaBind { 
    const char *symbol;
    luaFunc f;
};
const LuaBind bindings[] = {
    { "LoadResource", L_LoadResource },
    { "Print", L_Print },
};
const int nBindings = sizeof(bindings) / sizeof(LuaBind);

static const char *defaultSearchPaths[] = {
    "data",
    "data.zip",
};


bool Startup(void) {
    int i;
    if (!PHYSFS_init("argv[0]")) {
        fprintf(stderr, "PHYSFS_init failed");
        return false;
    }
    for (i = 0;i < sizeof(defaultSearchPaths) / sizeof(const char *);++i) {
        if (!PHYSFS_mount(defaultSearchPaths[i], "/", 1))
            fprintf(stderr, "WARNING: PHYSFS_mount failed to open %s: %s\n", defaultSearchPaths[i], PHYSFS_getLastError());
    }

    lState = luaL_newstate();
    for (i = 0;i < nBindings;++i) {
        lua_pushcfunction(lState, bindings[i].f);
        lua_setglobal(lState, bindings[i].symbol);
    }

    PHYSFS_file *fin;
    PHYSFS_sint64 fileLen;
    char *buf;

    fin = PHYSFS_openRead("autoexec.lua");
    if (nullptr == fin) {
        fprintf(stderr, "autoexec.lua is missing THIS IS REQUIRED\n");
        return false;
    }
    fileLen = PHYSFS_fileLength(fin);
    buf = new char[fileLen+1];
    PHYSFS_read(fin, buf, 1, fileLen);
    buf[fileLen] = '\0';
    if (LUA_FALSE == luaL_dostring(lState, buf)) {
        fprintf(stderr, "lua error: %s\n", lua_tostring(lState,-1));
        delete[] buf;
        return false;
    }
    fprintf(stderr, "autoexec.lua executed successfully\n");
    delete[] buf;
    return true;
}

void Shutdown(void) {
    lua_close(lState);
    PHYSFS_deinit();
}

static int L_LoadResource(lua_State *L) {
    if (lua_gettop(L) != 1)
        luaL_error(L, "Bad LoadResource() call");
    Texture t( lua_tostring(lState, 1) );
    return 0;
}

static int L_Print(lua_State *L) {
    int i, nArgs;

    nArgs = lua_gettop(L);
    for (i = 1;i <= nArgs;++i)
        fprintf(stderr, "%s", lua_tostring(L, i));
    lua_pop(L, nArgs);
    fflush(stderr);
    return 0;
}

} // ~namespace