#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include "componentlib.h"

static struct component *first_component = NULL;
static struct component *last_component = NULL;

static int component_list(lua_State *L) {
    if (!lua_isstring(L, 1)) {
        lua_newtable(L);
        return 1;
    }

    const char *filter = lua_tostring(L, 1);
    bool is_exact = lua_isboolean(L, 2) ? lua_toboolean(L, 2) : true;

    lua_newtable(L);

    for (struct component *component = first_component; component != NULL; component = component->next) {
        bool matches = !*filter || is_exact ? !strcmp(component->name, filter) : strstr(component->name, filter);

        if (!matches)
            continue;

        lua_pushstring(L, component->address);
        lua_pushstring(L, component->name);
        lua_rawset(L, -3);
    }

    return 1;
}

static int component_type(lua_State *L) {
    lua_pushnil(L);
    lua_pushstring(L, "component_type unimplemented");
    return 2;
}

static int component_slot(lua_State *L) {
    lua_pushnil(L);
    lua_pushstring(L, "component_slot unimplemented");
    return 2;
}

static int component_methods(lua_State *L) {
    const char *address = luaL_checkstring(L, 1);

    for (struct component *component = first_component; component != NULL; component = component->next)
        if (!strcmp(component->address, address)) {
            lua_newtable(L);

            for (struct method *method = component->first_method; method != NULL; method = method->next) {
                lua_pushstring(L, method->name);
                lua_newtable(L);
                lua_pushboolean(L, method->flags & METHOD_DIRECT);
                lua_setfield(L, -2, "direct");
                lua_pushboolean(L, method->flags & METHOD_GETTER);
                lua_setfield(L, -2, "getter");
                lua_pushboolean(L, method->flags & METHOD_SETTER);
                lua_setfield(L, -2, "setter");
                lua_rawset(L, -3);
            }

            return 1;
        }

    lua_pushnil(L);
    lua_pushstring(L, "no such component");
    return 2;
}

static int component_invoke(lua_State *L) {
    const char *address = luaL_checkstring(L, 1);
    const char *name = luaL_checkstring(L, 2);

    for (struct component *component = first_component; component != NULL; component = component->next)
        if (!strcmp(component->address, address)) {
            for (struct method *method = component->first_method; method != NULL; method = method->next)
                if (!strcmp(method->name, name)) {
                    lua_pushboolean(L, 1);
                    return method->invoke(L) + 1;
                }

            lua_pushnil(L);
            lua_pushstring(L, "no such method");
            return 2;
        }

    lua_pushnil(L);
    lua_pushstring(L, "no such component");
    return 2;
}

static int component_doc(lua_State *L) {
    lua_pushnil(L);
    lua_pushstring(L, "component_doc unimplemented");
    return 2;
}

static const luaL_Reg funcs[] = {
    {"list", component_list},
    {"type", component_type},
    {"slot", component_slot},
    {"methods", component_methods},
    {"invoke", component_invoke},
    {"doc", component_doc},
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
}

struct component *new_component(const char *name, const char *address) {
    struct component *component = malloc(sizeof(struct component));
    assert(component != NULL);
    assert(name != NULL);
    assert(address != NULL);

    component->name = name;
    component->address = address;
    component->first_method = NULL;
    component->last_method = NULL;

    return component;
}

void add_method(struct component *component, const char *name, int (*invoke)(lua_State *L), uint8_t flags) {
    struct method *method = malloc(sizeof(struct method));
    assert(method != NULL);
    assert(component != NULL);
    assert(name != NULL);

    method->name = name;
    method->invoke = invoke;
    method->next = NULL;
    method->flags = flags;

    if (component->last_method == NULL)
        component->first_method = component->last_method = method;
    else {
        component->last_method->next = method;
        component->last_method = method;
    }
}
