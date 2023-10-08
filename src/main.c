#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <malloc.h>
#include <assert.h>
#include <stdbool.h>
#include "multiboot.h"
#include "interrupts.h"
#include "rtc.h"
#include "tar.h"
#include "uuid.h"
#include "ps2.h"
#include "component/vgatext.h"
#include "component/vgagraphics.h"
#include "component/gpu.h"
#include "component/initrd.h"
#include "component/eeprom.h"
#include "api/component.h"
#include "api/computer.h"
#include "api/unicode.h"
#include "api/os.h"

extern uint32_t mboot_sig;
extern struct multiboot_header *mboot_ptr;

extern uint8_t kernel_start;
extern uint8_t kernel_end;

int luaopen_computer(lua_State *L);

static const luaL_Reg loadedlibs[] = {
    {LUA_GNAME, luaopen_base},
    {LUA_COLIBNAME, luaopen_coroutine},
    {LUA_TABLIBNAME, luaopen_table},
    {LUA_STRLIBNAME, luaopen_string},
    {LUA_MATHLIBNAME, luaopen_math},
    {LUA_UTF8LIBNAME, luaopen_utf8},
    {LUA_DBLIBNAME, luaopen_debug},
    {"computer", luaopen_computer},
    {"component", luaopen_component},
    {"unicode", luaopen_unicode},
    {"os", luaopen_os},
    {NULL, NULL}
};

uintptr_t memory_size = 0;

static char *message_traceback(lua_State *L) {
    luaL_traceback(L, L, lua_tostring(L, -1), 1);
    const char *message = lua_tostring(L, -1);
    lua_pop(L, 2);
    return message;
}

static const char *run_string(lua_State *L, const char *name, const char *data, size_t size) {
    if (size == 0)
        return;

    const char *message;

    if (luaL_loadbuffer(L, data, size, name))
        return message_traceback(L);

    while (1) {
        int nresults;

        if (lua_resume(L, NULL, 0, &nresults) == LUA_YIELD) {
            lua_pop(L, nresults);
            continue;
        } else
            return message_traceback(L);
    }
}

static int check_arg(lua_State *L) {
    int n = luaL_checkinteger(L, 1);
    const char *have = lua_typename(L, lua_type(L, 2));
    int top = lua_gettop(L);

    for (int i = 3; i <= top; i++)
        if (!strcmp(have, luaL_checkstring(L, i)))
            return 0;

    luaL_where(L, 2);
    lua_pushfstring(L, "bad argument #%d (", n);
    for (int i = 3; i <= top; i++) {
        lua_pushstring(L, luaL_checkstring(L, i));
        if (i < top) {
            lua_pushliteral(L, " or ");
            lua_concat(L, 3);
        } else
            lua_concat(L, 2);
    }
    lua_pushfstring(L, " expected, got %s)", have);
    lua_concat(L, 3);

    return lua_error(L);
}

void kmain(void) {
    printf("signature is %08x, flags are %08x\n", mboot_sig, mboot_ptr->flags);

    if (!(mboot_ptr->flags & (1 << 6))) {
        printf("don't have memory map info! aborting\n");
        return;
    }

    memory_size = (mboot_ptr->mem_lower + mboot_ptr->mem_upper) * 1024;
    printf("%d.%02d MiB of memory detected\n", memory_size / 1048576, (memory_size / 10485) % 100);
    printf("framebuffer type: %d\n", mboot_ptr->framebuffer_type);
    printf("framebuffer width: %d\n", mboot_ptr->framebuffer_width);
    printf("framebuffer height: %d\n", mboot_ptr->framebuffer_height);
    printf("framebuffer pitch: %d\n", mboot_ptr->framebuffer_pitch);
    printf("framebuffer bpp: %d\n",mboot_ptr->framebuffer_bpp);
    bool text_mode = mboot_ptr->framebuffer_type == 2;
    if (text_mode) {
        puts("text mode? yes");
    } else {
        puts("text mode? no");
    }
    uintptr_t largest_size = 0;
    uintptr_t base_addr = 0;
    struct mmap_entry *mmap = mboot_ptr->mmap_addr;

    for (int i = 0; i < mboot_ptr->mmap_length && mmap->size > 0; i += mmap->size + 4) {
        mmap = mboot_ptr->mmap_addr + i;
        printf("%016llx + %016llx, type %d\n", mmap->base_addr, mmap->length, mmap->type);

        if (mmap->type != AVAILABLE_RAM)
            continue;

        if ((uintptr_t) mmap->length <= largest_size || mmap->base_addr > UINTPTR_MAX || mmap->length > UINTPTR_MAX)
            continue;

        uintptr_t tmp;
        if (__builtin_add_overflow((uintptr_t) mmap->base_addr, (uintptr_t) mmap->length, &tmp))
            continue;

        largest_size = mmap->length;
        base_addr = mmap->base_addr;
    }

    if ((uintptr_t) &kernel_start >= base_addr || base_addr < (uintptr_t) &kernel_end) {
        uintptr_t reduction = (uintptr_t) &kernel_end - base_addr;
        assert(reduction < largest_size);
        largest_size -= reduction;
        base_addr = (uintptr_t) &kernel_end;
    }

    uintptr_t header_end = (uintptr_t) mboot_ptr + sizeof(struct multiboot_header);
    if ((uintptr_t) mboot_ptr >= base_addr || base_addr < header_end) {
        uintptr_t reduction = (uintptr_t) header_end - base_addr;
        assert(reduction < largest_size);
        largest_size -= reduction;
        base_addr = header_end;
    }

    printf("%d module(s) at %08x\n", mboot_ptr->mods_count, mboot_ptr->mods_addr);

    struct module_entry *module = mboot_ptr->mods_addr;

    //for (int i = 0; i < mboot_ptr->mods_count; i ++, module ++) {
    if (mboot_ptr->mods_count) {
        uintptr_t size = (uintptr_t) module->end - (uintptr_t) module->start;
        printf("%08x - %08x (%d.%02d KiB): \"%s\"\n", module->start, module->end, size / 1024, (size / 10) % 100, module->string);

        if ((uintptr_t) module->start >= base_addr || base_addr < (uintptr_t) module->end) {
            uintptr_t reduction = (uintptr_t) module->end - base_addr;
            assert(reduction < largest_size);
            largest_size -= reduction;
            base_addr = (uintptr_t) module->end;
        }
    }

    printf("using block at %08x + %08x (%d.%02d MiB) for alloc\n", base_addr, largest_size, largest_size / 1048576, (largest_size / 10485) % 100);
    malloc_addblock(base_addr, largest_size);

    init_idt();

    printf("testing interrupts\n");
    __asm__ __volatile__ ("int3");

    rtc_init();
    printf("time is %lld\n", epoch_time);
    srand(epoch_time);

    printf("initializing components\n");
    struct eeprom_data *eeprom = eeprom_init();
    struct gpu *gpu;
    if (text_mode) {
        gpu = vgatext_init();
    } else {
        gpu = vgagraphics_init();
    }
    printf("gpu characteristics:\n");
    printf("\taddr: %p\n",gpu);
    printf("\twidth: %d\n",gpu->width);
    printf("\theight: %d\n",gpu->height);
    printf("\tdepth: %d\n",gpu->depth);
    printf("\tpalette size: %d\n",gpu->palette_size);
    printf("\tpalette: %p\n",gpu->palette);
    printf("\tset: %p\n",gpu->set);
    printf("\tcopy: %p\n",gpu->copy);

    if (mboot_ptr->mods_count != 0)
        initrd_init(mboot_ptr->mods_addr->string, mboot_ptr->mods_addr->start, mboot_ptr->mods_addr->end);

    ps2_init();

    printf("starting Lua\n");
    lua_State *L = luaL_newstate();
    assert(L != NULL);

    const luaL_Reg *lib;
    /* "require" functions from 'loadedlibs' and set results to global table */
    for (lib = loadedlibs; lib->func; lib++) {
        printf("loading library %s\n", lib->name);
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* remove lib */
    }

    lua_register(L, "checkArg", check_arg);

    if (mboot_ptr->mods_count != 0) {
        module = mboot_ptr->mods_addr;

        struct tar_iterator *iter;
        const char *data;
        size_t size;
        if (!text_mode) {
            printf("loading font\n");
            iter = open_tar(module->start, module->end);
            
            if (tar_find(iter,"/font.hex",TAR_NORMAL_FILE,&data, &size)) {
                vgagraphics_load_font(data,size);
            } else 
                gpu_error_message(gpu, "could not find font.hex");
            
            size = 0;
            data = NULL;
        }
        iter = open_tar(module->start, module->end);
        if (tar_find(iter, "/bios.lua", TAR_NORMAL_FILE, &data, &size)) {
            printf("running bios.lua\n");
            eeprom->contents = data;
            gpu_error_message(gpu, run_string(L, "=bios.lua", data, size));
        } else
            gpu_error_message(gpu, "couldn't find bios.lua");
    } else
        gpu_error_message(gpu, "missing initrd");

    printf("finished execution, halting\n");
    lua_close(L);

    while (1)
        __asm__ __volatile__ ("cli; hlt");
}
