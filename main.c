#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <malloc.h>
#include "multiboot.h"

extern uint32_t mboot_sig;
extern struct multiboot_header *mboot_ptr;

extern uint8_t kernel_start;
extern uint8_t kernel_end;

void kmain(void) {
    printf("signature is %08x, flags are %08x\n", mboot_sig, mboot_ptr->flags);

    if (!(mboot_ptr->flags & (1 << 6))) {
        printf("don't have memory map info! aborting\n");
        return;
    }

    uint32_t largest_size = 0;
    uint32_t base_addr = 0;
    struct mmap_entry *entry = mboot_ptr->mmap_addr;

    for (int i = 0; i < mboot_ptr->mmap_length && entry->size > 0; i += entry->size + 4) {
        entry = mboot_ptr->mmap_addr + i;
        printf("%016llx + %016llx, type %d\n", entry->base_addr, entry->length, entry->type);

        if (entry->type != AVAILABLE_RAM)
            continue;

        if ((uint32_t) entry->length <= largest_size)
            continue;

        largest_size = entry->length;
        base_addr = entry->base_addr;
    }

    if (&kernel_start >= base_addr || base_addr < &kernel_end) {
        largest_size -= (size_t) &kernel_end - base_addr;
        base_addr = &kernel_end;
    }

    printf("using block at %08x + %08x (%d MiB) for alloc\n", base_addr, largest_size, largest_size / 1024 / 1024);
    malloc_addblock(base_addr, largest_size);

    init_idt();

    printf("testing interrupts\n");
    __asm__ __volatile__ ("int3");

    const char *buff = "print(\"UwU OwO\")\n";
    int error;
    const luaL_Reg *lib;
    lua_State *L = luaL_newstate();

    luaL_requiref(L, LUA_GNAME, luaopen_base, 1);
    lua_pop(L, 1);

    error = luaL_loadbuffer(L, buff, strlen(buff), "line") || lua_pcall(L, 0, 0, 0);
    if (error) {
        printf("%s\n", lua_tostring(L, -1));
        lua_pop(L, 1);  /* pop error message from the stack */
    }

    lua_close(L);

    while (1);
}
