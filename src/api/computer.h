#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <lua.h>

struct signal {
    const char *name;
    uint8_t kind;
    union {
        struct {
            const char *name;
            size_t size;
        } registry;
        struct {
            const char *address;
            uint32_t character;
            uint32_t code;
        } keyboard;
    } data;
};

#define SIG_LUA 0
#define SIG_KEYBOARD 1

bool queue_signal(struct signal *signal);
int luaopen_computer(lua_State *L);
