#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <malloc.h>
#include "multiboot.h"
#include "interrupts.h"

extern uint32_t mboot_sig;
extern struct multiboot_header *mboot_ptr;

extern uint8_t kernel_start;
extern uint8_t kernel_end;

static const luaL_Reg loadedlibs[] = {
    {LUA_GNAME, luaopen_base},
    {LUA_COLIBNAME, luaopen_coroutine},
    {LUA_TABLIBNAME, luaopen_table},
    {LUA_STRLIBNAME, luaopen_string},
    {LUA_MATHLIBNAME, luaopen_math},
    {LUA_UTF8LIBNAME, luaopen_utf8},
    {LUA_DBLIBNAME, luaopen_debug},
    {NULL, NULL}
};

void kmain(void) {
    printf("signature is %08x, flags are %08x\n", mboot_sig, mboot_ptr->flags);

    if (!(mboot_ptr->flags & (1 << 6))) {
        printf("don't have memory map info! aborting\n");
        return;
    }

    uint32_t largest_size = 0;
    uint32_t base_addr = 0;
    struct mmap_entry *mmap = mboot_ptr->mmap_addr;

    for (int i = 0; i < mboot_ptr->mmap_length && mmap->size > 0; i += mmap->size + 4) {
        mmap = mboot_ptr->mmap_addr + i;
        printf("%016llx + %016llx, type %d\n", mmap->base_addr, mmap->length, mmap->type);

        if (mmap->type != AVAILABLE_RAM)
            continue;

        if ((uint32_t) mmap->length <= largest_size)
            continue;

        largest_size = mmap->length;
        base_addr = mmap->base_addr;
    }

    if ((uint32_t) &kernel_start >= base_addr || base_addr < (uint32_t) &kernel_end) {
        largest_size -= (uint32_t) &kernel_end - base_addr;
        base_addr = (uint32_t) &kernel_end;
    }

    uint32_t header_end = (uint32_t) mboot_ptr + sizeof(struct multiboot_header);
    if ((uint32_t) mboot_ptr >= base_addr || base_addr < header_end) {
        largest_size -= (uint32_t) header_end - base_addr;
        base_addr = header_end;
    }

    printf("%d module(s) at %08x\n", mboot_ptr->mods_count, mboot_ptr->mods_addr);

    struct module_entry *module = mboot_ptr->mods_addr;

    for (int i = 0; i < mboot_ptr->mods_count; i ++, module ++) {
        printf("%08x - %08x: \"%s\"\n", module->start, module->end, module->string);

        if ((uint32_t) module->start >= base_addr || base_addr < (uint32_t) module->end) {
            largest_size -= (uint32_t) module->end - base_addr;
            base_addr = (uint32_t) module->end;
        }
    }

    printf("using block at %08x + %08x (%d.%02d MiB) for alloc\n", base_addr, largest_size, largest_size / 1048576, (largest_size / 10485) % 100);
    malloc_addblock(base_addr, largest_size);

    init_idt();

    printf("testing interrupts\n");
    __asm__ __volatile__ ("int3");

    printf("starting Lua\n");
    const char *buff = "print(\"UwU OwO\")\n";
    int error;
    lua_State *L = luaL_newstate();

    const luaL_Reg *lib;
    /* "require" functions from 'loadedlibs' and set results to global table */
    for (lib = loadedlibs; lib->func; lib++) {
        printf("loading library %s\n", lib->name);
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* remove lib */
    }

    if (mboot_ptr->mods_count == 0)
        printf("nothing to run!\n");
    else {
        module = mboot_ptr->mods_addr;
        printf("running module \"%s\"\n", module->string);
        error = luaL_loadbuffer(L, module->start, module->end - module->start, module->string) || lua_pcall(L, 0, 0, 0);
        if (error)
            printf("%s\n", lua_tostring(L, -1));
    }

    lua_close(L);

    while (1);
}
