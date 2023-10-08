#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include "rtc.h"
#include "uuid.h"
#include "io.h"
#include "api/computer.h"

static const char *address;

static int get_real_time(lua_State *L) {
    lua_pushnumber(L, epoch_time);
    return 1;
}

static int get_uptime(lua_State *L) {
    lua_pushnumber(L, uptime);
    return 1;
}

static int get_address(lua_State *L) {
    lua_pushstring(L, address);
    return 1;
}

#define MAX_SIGNALS 128

static volatile struct signal signal_buffer[MAX_SIGNALS];
static volatile int in_buffer = 0;
static volatile int buffer_pos = 0;

// adds a signal to the queue
bool queue_signal(struct signal *signal) {
    assert(signal != NULL);

    __asm__ __volatile__ ("cli");

    if (in_buffer >= MAX_SIGNALS) {
        __asm__ __volatile__ ("sti");
        return false;
    }

    memcpy(&signal_buffer[(buffer_pos + in_buffer) % MAX_SIGNALS], signal, sizeof(struct signal));
    in_buffer ++;

    __asm__ __volatile__ ("sti");
    return true;
}

// removes a signal from the queue
static bool dequeue_signal(struct signal *signal) {
    assert(signal != NULL);

    if (in_buffer == 0)
        return false;

    __asm__ __volatile__ ("cli");

    memcpy(signal, &signal_buffer[buffer_pos], sizeof(struct signal));

    in_buffer --;
    buffer_pos = (buffer_pos + 1) % MAX_SIGNALS;

    __asm__ __volatile__ ("sti");
    return true;
}

// frees memory allocated for a signal
static void free_signal(struct lua_State *L, struct signal *signal) {
    if (signal->kind == SIG_LUA) {
        free(signal->name);

        if (signal->data.registry.name != NULL) {
            // delete arguments from registry
            lua_pushstring(L, signal->data.registry.name);
            lua_pushnil(L);
            lua_settable(L, LUA_REGISTRYINDEX);

            free(signal->data.registry.name);
        }
    }
}

static int push_signal(lua_State *L) {
    size_t len;
    const char *name = luaL_checklstring(L, 1, &len);
    int arguments = lua_gettop(L) - 1;

    const char *registry_name = NULL;

    if (arguments > 0)
        registry_name = new_uuid(); // this should be good enough to prevent collisions

    struct signal signal = {
        .name = strdup(name),
        .kind = SIG_LUA,
        .data = {
            .registry = {
                .name = registry_name,
                .size = arguments
            }
        }
    };

    if (signal.name == NULL)
        return luaL_error(L, "out of memory");

    if (arguments > 0) {
        // store all the arguments in a table in the global C registry
        lua_pushstring(L, registry_name);
        lua_newtable(L);

        for (int i = 0; i < arguments; i ++) {
            lua_pushinteger(L, i);
            lua_pushnil(L);
            lua_copy(L, i + 2, lua_gettop(L));
            lua_rawset(L, -3);
        }

        lua_settable(L, LUA_REGISTRYINDEX);
    }

    if (!queue_signal(&signal)) {
        free_signal(L, &signal);
        return luaL_error(L, "too many signals");
    }

    return 0;
}

static int pull_signal(lua_State *L) {
    // ignored for now
    uint32_t timeout = lua_isinteger(L, 1) ? lua_tointeger(L, 1) : -1;
    struct signal signal;

    if (!dequeue_signal(&signal))
        return 0;

    size_t arguments = 0;

    lua_pushstring(L, signal.name);

    switch (signal.kind) {
        case SIG_LUA:
            if ((arguments = signal.data.registry.size) > 0) {
                // get the arguments from the registry
                lua_pushstring(L, signal.data.registry.name);
                lua_gettable(L, LUA_REGISTRYINDEX);
                int table_index = lua_gettop(L);

                // extract all the arguments from the table
                for (int i = 0; i < arguments; i ++) {
                    lua_pushinteger(L, i);
                    lua_rawget(L, table_index);
                }

                // remove the table from the stack since it's not needed
                lua_remove(L, table_index);
            }
            break;
        case SIG_KEYBOARD:
            arguments = 4;
            lua_pushstring(L, signal.data.keyboard.address);
            lua_pushinteger(L, signal.data.keyboard.character);
            lua_pushinteger(L, signal.data.keyboard.code);
            lua_pushliteral(L, "");
            break;
    }

    free_signal(L, &signal);

    return arguments + 1;
}

static int get_tmp_address(lua_State *L) {
    lua_pushnil(L);
    return 1;
}

static void delay(lua_Number duration) {
    uint64_t seconds = (int) duration;
    uint16_t jiffies = (int) (duration * 1024.0) % 1024;

    uint64_t deadline_seconds = uptime + seconds;
    uint16_t deadline_jiffies = jiffies_frac + jiffies;
    deadline_seconds += deadline_jiffies / 1024;
    deadline_jiffies %= 1024;

    while (uptime < deadline_seconds || jiffies_frac < deadline_jiffies);
}

static void beep(int frequency, lua_Number duration) {
    int div = 1193180 / frequency;
    outb(0x43, 0xb6);
    outb(0x42, div & 0xff);
    outb(0x42, (div >> 8) & 0xff);

    uint8_t tmp = inb(0x61);
    if (tmp != (tmp | 3))
        outb(0x61, tmp | 3);

    delay(duration);

    tmp = inb(0x61) & 0xfc;
    outb(0x61, tmp);
}

static int computer_beep(lua_State *L) {
    if (lua_isnumber(L, 1)) {
        int frequency = lua_isnumber(L, 1) ? lua_tointeger(L, 1) : 440;
        lua_Number duration = lua_isnumber(L, 2) ? lua_tonumber(L, 2) : 0.1;

        beep(frequency, duration);
    } else {
        const char *s = luaL_checkstring(L, 1);
        
        for (; *s; s++) {
            if (*s == '.')
                beep(1000, 0.1);
            else
                beep(1000, 0.2);

            delay(0.05);
        }
    }

    return 0;
}

extern uintptr_t memory_size;

static int total_memory(lua_State *L) {
    lua_pushinteger(L, memory_size);
    return 1;
}

static const luaL_Reg funcs[] = {
    {"realTime", get_real_time},
    {"uptime", get_uptime},
    {"address", get_address},
    {"pushSignal", push_signal},
    {"pullSignal", pull_signal},
    {"tmpAddress", get_tmp_address},
    {"beep", computer_beep},
    {"totalMemory", total_memory},
    {"freeMemory", total_memory},
    {NULL, NULL}
};

int luaopen_computer(lua_State *L) {
    address = new_uuid();
    struct component *computer = new_component("computer", address, NULL);
    add_component(computer);

    luaL_newlib(L, funcs);

    return 1;
}
