#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include "api/component.h"

static struct component *first_component = NULL;
static struct component *last_component = NULL;

static int list_call(lua_State *L) {
    lua_pushnil(L);
    lua_copy(L, lua_upvalueindex(2), -1);

    if (!lua_next(L, lua_upvalueindex(1)))
        return 0;

    lua_copy(L, -2, lua_upvalueindex(2));
    return 2;
}

static int component_list(lua_State *L) {
    const char *filter = lua_isstring(L, 1) ? lua_tostring(L, 1) : "";
    bool is_exact = lua_isboolean(L, 2) ? lua_toboolean(L, 2) : true;

    lua_newtable(L);

    for (struct component *component = first_component; component != NULL; component = component->next) {
        if (*filter && (is_exact ? strcmp(component->name, filter) : !strstr(component->name, filter)))
            continue;

        lua_pushstring(L, component->name);
        lua_setfield(L, -2, component->address);
    }

    lua_newtable(L);

    lua_pushnil(L);
    lua_copy(L, -3, -1);
    lua_pushnil(L);
    lua_pushcclosure(L, list_call, 2);
    lua_setfield(L, -2, "__call");

    lua_setmetatable(L, -2);

    return 1;
}

static int component_type(lua_State *L) {
    const char *address = luaL_checkstring(L, 1);

    for (struct component *component = first_component; component != NULL; component = component->next)
        if (!strcmp(component->address, address)) {
            lua_pushstring(L, component->name);
            return 1;
        }

    return luaL_error(L, "no such component");
}

static int component_slot(lua_State *L) {
    lua_pushnumber(L, -1);
    return 1;
}

static int component_methods(lua_State *L) {
    const char *address = luaL_checkstring(L, 1);

    for (struct component *component = first_component; component != NULL; component = component->next)
        if (!strcmp(component->address, address)) {
            lua_newtable(L);

            for (struct method *method = component->first_method; method != NULL; method = method->next) {
                lua_pushboolean(L, true);
                lua_setfield(L, -2, method->name);
            }

            return 1;
        }

    return luaL_error(L, "no such component");
}

static int component_invoke(lua_State *L) {
    const char *address = luaL_checkstring(L, 1);
    const char *name = luaL_checkstring(L, 2);

    for (struct component *component = first_component; component != NULL; component = component->next)
        if (!strcmp(component->address, address)) {
            for (struct method *method = component->first_method; method != NULL; method = method->next)
                if (!strcmp(method->name, name))
                    return method->invoke(L, component->data, 3);

            return luaL_error(L, "no such method");
        }

    return luaL_error(L, "no such component");
}

static int component_doc(lua_State *L) {
    lua_pushnil(L);
    return 1;
}

static int proxy_call(lua_State *L) {
    void *data = (void *) ((uintptr_t) lua_tointeger(L, lua_upvalueindex(1)));
    struct method *method = (struct method *) ((uintptr_t) lua_tointeger(L, lua_upvalueindex(2)));
    return method->invoke(L, data, 1);
}

static int component_proxy(lua_State *L) {
    const char *address = luaL_checkstring(L, 1);

    for (struct component *component = first_component; component != NULL; component = component->next)
        if (!strcmp(component->address, address)) {
            lua_newtable(L);

            lua_pushstring(L, address);
            lua_setfield(L, -2, "address");
            lua_pushstring(L, component->name);
            lua_setfield(L, -2, "type");

            for (struct method *method = component->first_method; method != NULL; method = method->next) {
                lua_pushinteger(L, (uintptr_t) component->data);
                lua_pushinteger(L, (uintptr_t) method); // only slightly cursed :3
                lua_pushcclosure(L, proxy_call, 2);
                lua_setfield(L, -2, method->name);
            }

            return 1;
        }

    return luaL_error(L, "no such component");
}

static const luaL_Reg funcs[] = {
    {"list", component_list},
    {"type", component_type},
    {"slot", component_slot},
    {"methods", component_methods},
    {"invoke", component_invoke},
    {"doc", component_doc},
    {"proxy", component_proxy},
    {NULL, NULL}
};

int luaopen_component(lua_State *L) {
    luaL_newlib(L, funcs);
    return 1;
}

void add_component(struct component *component) {
    assert(component != NULL);

    component->next = NULL;

    if (last_component == NULL)
        first_component = last_component = component;
    else {
        last_component->next = component;
        last_component = component;
    }

    printf("added \"%s\" component at %s\n", component->name, component->address);
}

struct component *new_component(const char *name, const char *address, void *data) {
    struct component *component = malloc(sizeof(struct component));
    assert(component != NULL);
    assert(name != NULL);
    assert(address != NULL);

    component->name = name;
    component->address = address;
    component->data = data;
    component->first_method = NULL;
    component->last_method = NULL;

    return component;
}

void add_method(struct component *component, const char *name, int (*invoke)(lua_State *L, void *data, int arguments_start)) {
    struct method *method = malloc(sizeof(struct method));
    assert(method != NULL);
    assert(component != NULL);
    assert(name != NULL);

    method->name = name;
    method->invoke = invoke;
    method->next = NULL;

    if (component->last_method == NULL)
        component->first_method = component->last_method = method;
    else {
        component->last_method->next = method;
        component->last_method = method;
    }
}
