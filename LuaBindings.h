// Copyright [year] <Copyright Owner>
#ifndef _LUABINDINGS_H_
#define _LUABINDINGS_H_

namespace Lua {

#include <lua/lua.hpp>
bool Startup(void);
void Shutdown(void);

// use these with caution
// TODO: Think of how to break the dependency
bool LuaExec(const char *expr);
extern lua_State *lState;
}
#endif // ~_LUABINDINGS_H_