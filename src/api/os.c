#include <lua.h>
#include <lauxlib.h>
#include "os.h"
#include "rtc.h"

static int os_clock(lua_State *L) {
    lua_pushnumber(L, uptime);
    return 1;
}

static const luaL_Reg funcs[] = {
    {"clock", os_clock},
    {NULL, NULL}
};

int luaopen_os(lua_State *L) {
    luaL_newlib(L, funcs);
    return 1;
}
