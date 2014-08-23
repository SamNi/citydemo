// Copyright [year] <Copyright Owner>
#ifndef _LUABINDINGS_H_
#define _LUABINDINGS_H_
#include "essentials.h"
#include <lua/lua.hpp>

struct Lua {
    static bool startup(void);
    static void shutdown(void);
    static bool exec(const char *expression);

    static lua_State* get_state(void);
};

#endif // ~_LUABINDINGS_H_