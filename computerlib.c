#include <lua.h>
#include <lauxlib.h>
#include "rtc.h"

static int get_real_time(lua_State *L) {
    lua_pushnumber(L, epoch_time);
    return 1;
}

static int get_uptime(lua_State *L) {
    lua_pushnumber(L, uptime);
    return 1;
}

static const luaL_Reg funcs[] = {
    {"realTime", get_real_time},
    {"uptime", get_uptime},
    {NULL, NULL}
};

int luaopen_computer(lua_State *L) {
    luaL_newlib(L, funcs);
    return 1;
}
