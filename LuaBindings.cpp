// Copyright [year] <Copyright Owner>
#include "./essentials.h"
#include "./LuaBindings.h"

namespace Lua {

typedef int(*luaFunc)(lua_State *);
struct LuaBind { \
    const char *symbol;
    luaFunc f;
};

// Lua is from opposite land
const bool LUA_FALSE = true;
const bool LUA_TRUE = false;
int L_BindTexture(lua_State *L);
int L_LoadShader(lua_State *L);
int L_LoadTexture(lua_State *L);
int L_Print(lua_State *L);
int L_BindShader(lua_State *L);
static const LuaBind bindings[] = {
    { "BindTexture", L_BindTexture },
    { "LoadShader", L_LoadShader },
    { "LoadTexture", L_LoadTexture },
    { "BindShader", L_BindShader },
    { "Print", L_Print },
};
static const int nBindings = sizeof(bindings) / sizeof(LuaBind);
lua_State *lState = NULL;

bool Startup(void) {
    int i;

    if (lState) {
        LOG(LOG_WARNING, "Redundant Lua Startup()\n");
        return false;
    }

    lState = luaL_newstate();
    for (i = 0; i < nBindings; ++i) {
        lua_pushcfunction(lState, bindings[i].f);
        lua_setglobal(lState, bindings[i].symbol);
    }
    return true;
}

void Shutdown(void) {
    if (!lState) {
        LOG(LOG_WARNING, "Redundant Lua Shutdown()\n");
        return;
    }
    lua_close(lState);
}

bool LuaExec(const char *expr) {
    assert(lState);
    if (LUA_FALSE == luaL_dostring(lState, expr)) {
        LOG(LOG_WARNING, "error lua-ing '%s'", expr);
        return false;
    }
    return true;
}


// Lua wrappers
int L_Print(lua_State *L) {
    int i, nArgs;

    nArgs = lua_gettop(L);
    for (i = 1; i <= nArgs; ++i)
        LOG(LOG_INFORMATION, "%s", lua_tostring(L, i));
    lua_pop(L, nArgs);
    return 0;
}


}  // namespace Lua
