#include <stdbool.h>
#include <lauxlib.h>
#include <lua.h>
#include "api/unicode.h"

// too lazy to copy and paste
#define luaopen_utf8 __luaopen_utf8
#define funcs __funcs
#include "../../lua/lutf8lib.c"
#undef luaopen_utf8
#undef funcs

static lua_Integer string_length(const char *s, size_t len) {
    if (!*s)
        return 0;

    const char *s1;
    lua_Integer i = 0, n = 0;

    for (; i < len; i = s1 - s, n++) {
        s1 = utf8_decode(s + i, NULL, false);
        if (s1 == NULL)
            return -1;
    }

    return n;
}

static int unicode_len(lua_State *L) {
    size_t len;
    const char *s = luaL_checklstring(L, 1, &len);

    int actual_len = string_length(s, len);

    if (actual_len >= 0) {
        lua_pushinteger(L, actual_len);
        return 1;
    } else
        return luaL_error(L, "invalid UTF-8");
}

/* TODO */
static int unicode_char_width(lua_State *L) {
    size_t len;
    luaL_checklstring(L, 1, &len);
    lua_pushnumber(L, len > 0);
    return 1;
}

static int unicode_is_wide(lua_State *L) {
    lua_pushboolean(L, false);
    return 1;
}

static int unicode_wtrunc(lua_State *L) {
    luaL_Buffer b;
    const char *string = luaL_checkstring(L, 1);
    size_t width = luaL_checkinteger(L, 2);

    luaL_buffinit(L, &b);

    for (size_t i = 0; i < width && *string; i++) {
        utfint c;
        if ((string = utf8_decode(string, &c, false)) == NULL)
            return luaL_error(L, "invalid UTF-8");

        lua_pushfstring(L, "%U", (long) c);
        luaL_addvalue(&b);
    }

    luaL_pushresult(&b);
    return 1;
}

static lua_Integer offset_to_bytes(const char *s, size_t len, lua_Integer n) {
    lua_Integer posi = 0;

    if (s == NULL || len == 0 || n <= 0)
        return 0;

    while (n > 0 && posi < (lua_Integer) len) {
        do {  /* find beginning of next character */
            posi++;
        } while (iscont(s + posi));  /* (cannot pass final '\0') */
        n--;
    }

    return posi;
}

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

static int unicode_sub(lua_State *L) {
    size_t len;
    const char *s = luaL_checklstring(L, 1, &len);

    // incredibly naive implementation translated from OpenComputers' source code

    lua_Integer s_length = string_length(s, len);
    if (s_length < 0)
        return luaL_error(L, "invalid UTF-8");

    lua_Integer start = luaL_checkinteger(L, 2);

    if (start < 0)
        start = offset_to_bytes(s, len, MAX(start, -s_length));
    else if (start > 0)
        start = offset_to_bytes(s, len, MIN(start - 1, s_length));

    lua_Integer end = (lua_Integer) len;

    if (lua_isinteger(L, 3)) {
        end = lua_tointeger(L, 3);

        if (end < 0)
            end = offset_to_bytes(s, len, MAX(end + 1, -s_length));
        else
            end = offset_to_bytes(s, len, MIN(end, s_length));
    }

    if (end <= start)
        lua_pushliteral(L, "");
    else
        lua_pushlstring(L, s + start, end - start);

    return 1;
}

static const luaL_Reg funcs[] = {
    {"char", utfchar},
    {"charWidth", unicode_char_width},
    {"isWide", unicode_is_wide},
    {"len", unicode_len},
    // lower
    // reverse
    {"sub", unicode_sub},
    // upper
    {"wlen", unicode_len},
    {"wtrunc", unicode_wtrunc},
    {NULL, NULL}
};

int luaopen_unicode(lua_State *L) {
    luaL_newlib(L, funcs);
    return 1;
}
