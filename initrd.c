#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include "initrd.h"
#include "componentlib.h"
#include "uuid.h"
#include "tar.h"

struct initrd_data {
    const char *name;
    const char *start;
    const char *end;
    struct open_file *open_files;
};

struct open_file {
    int handle_num;
    const char *start;
    size_t size;
    size_t read_pos;

    struct open_file *next;
};

static int initrd_space_used(lua_State *L, const char *address, struct initrd_data *data) {
    lua_pushboolean(L, true);
    lua_pushnumber(L, data->end - data->start);
    return 2;
}

static int initrd_open(lua_State *L, const char *address, struct initrd_data *data) {
    const char *path = luaL_checkstring(L, 3);
    const char *mode = lua_isstring(L, 4) ? lua_tostring(L, 4) : "r";

    if (!*path || !*mode) {
        lua_pushboolean(L, false);
        lua_pushliteral(L, "bad argument");
        return 2;
    }

    if (strstr(mode, "r") == NULL || strstr(mode, "w") != NULL) {
        lua_pushboolean(L, false);
        lua_pushliteral(L, "read-only filesystem");
        return 2;
    }

    struct tar_iterator *iter = open_tar(data->start, data->end);
    const char *file_data;
    size_t file_size;
    if (!find_file(iter, path, &file_data, &file_size)){
        lua_pushboolean(L, false);
        lua_pushliteral(L, "file not found");
        return 2;
    }

    struct open_file *open_file = malloc(sizeof(struct open_file));
    assert(open_file != NULL);
    open_file->start = file_data;
    open_file->size = file_size;
    open_file->read_pos = 0;

    if (data->open_files == NULL) {
        open_file->handle_num = 0;
        open_file->next = NULL;
    } else {
        open_file->handle_num = data->open_files->handle_num + 1;
        open_file->next = data->open_files;
    }

    data->open_files = open_file;

    lua_pushboolean(L, true);
    lua_pushnumber(L, open_file->handle_num);
    return 2;
}

static int initrd_readonly(lua_State *L, const char *address, struct initrd_data *data) {
    lua_pushboolean(L, false);
    lua_pushliteral(L, "read-only filesystem");
    return 2;
}

static struct open_file *find_open_file(struct initrd_data *data, int handle) {
    for (struct open_file *open_file = data->open_files; open_file != NULL; open_file = open_file->next)
        if (open_file->handle_num == handle)
            return open_file;

    return NULL;
}

static int initrd_seek(lua_State *L, const char *address, struct initrd_data *data) {
    int handle = luaL_checknumber(L, 3);
    const char *whence = luaL_checkstring(L, 4);
    int offset = luaL_checknumber(L, 5);

    struct open_file *open_file = find_open_file(data, handle);

    if (open_file == NULL) {
        lua_pushboolean(L, false);
        lua_pushliteral(L, "invalid handle");
        return 2;
    }

    if (!strcmp(whence, "cur")) {
        open_file->read_pos += offset;
    } else if (!strcmp(whence, "set")) {
        open_file->read_pos = offset;
    } else if (!strcmp(whence, "end")) {
        open_file->read_pos = open_file->size + offset;
    }

    lua_pushboolean(L, true);
    lua_pushnumber(L, open_file->read_pos);
    return 2;
}

static int initrd_exists(lua_State *L, const char *address, struct initrd_data *data) {
    const char *path = luaL_checkstring(L, 3);

    if (!*path) {
        lua_pushboolean(L, true);
        lua_pushboolean(L, false);
        return 2;
    }

    struct tar_iterator *iter = open_tar(data->start, data->end);
    const char *file_data;
    size_t file_size;

    lua_pushboolean(L, true);
    lua_pushboolean(L, find_file(iter, path, &file_data, &file_size));
    return 2;
}

static int initrd_is_read_only(lua_State *L, const char *address, void *data) {
    lua_pushboolean(L, true);
    lua_pushboolean(L, true);
    return 2;
}

static int initrd_get_label(lua_State *L, const char *address, struct initrd_data *data) {
    lua_pushboolean(L, true);
    lua_pushstring(L, data->name);
    return 2;
}

static int initrd_size(lua_State *L, const char *address, struct initrd_data *data) {
    const char *path = luaL_checkstring(L, 3);

    if (!*path) {
        lua_pushboolean(L, true);
        lua_pushboolean(L, false);
        return 2;
    }

    struct tar_iterator *iter = open_tar(data->start, data->end);
    const char *file_data;
    size_t file_size;
    if (!find_file(iter, path, &file_data, &file_size)){
        lua_pushboolean(L, false);
        lua_pushliteral(L, "file not found");
        return 2;
    }

    lua_pushboolean(L, true);
    lua_pushnumber(L, file_size);
    return 2;
}

static int initrd_close(lua_State *L, const char *address, struct initrd_data *data) {
    int handle = luaL_checknumber(L, 3);

    struct open_file *open_file = data->open_files, *last = NULL;

    for (; open_file != NULL; last = open_file, open_file = open_file->next)
        if (open_file->handle_num == handle) {
            if (last == NULL)
                data->open_files = open_file->next;
            else
                last->next = open_file->next;

            free(open_file);

            lua_pushboolean(L, true);
            return 1;
        }

    lua_pushboolean(L, false);
    lua_pushliteral(L, "invalid handle");
    return 2;
}

static int initrd_read(lua_State *L, const char *address, struct initrd_data *data) {
    int handle = luaL_checknumber(L, 3);

    // lua is on crack
    uint64_t count2 = luaL_checknumber(L, 4);
    size_t count = count2 >= UINTPTR_MAX ? UINTPTR_MAX : count;

    struct open_file *open_file = find_open_file(data, handle);

    if (open_file == NULL) {
        lua_pushboolean(L, false);
        lua_pushliteral(L, "invalid handle");
        return 2;
    }

    if (open_file->read_pos >= open_file->size) {
        lua_pushboolean(L, true);
        lua_pushnil(L);
        return 2;
    }

    if (count > open_file->size)
        count = open_file->size;

    if (open_file->read_pos + count >= open_file->size)
        count -= open_file->size - (open_file->read_pos + count);
    
    if (count == 0) {
        lua_pushboolean(L, true);
        lua_pushnil(L);
        return 2;
    }

    lua_pushboolean(L, true);
    lua_pushlstring(L, open_file->start + open_file->read_pos, count);

    open_file->read_pos += count;

    return 2;
}

void initrd_init(const char *name, const char *start, const char *end) {
    struct initrd_data *data = malloc(sizeof(struct initrd_data));
    assert(data != NULL);

    data->name = name;
    data->start = start;
    data->end = end;

    struct component *filesystem = new_component("filesystem", new_uuid(), data);
    add_method(filesystem, "spaceUsed", initrd_space_used, METHOD_DIRECT | METHOD_GETTER);
    add_method(filesystem, "open", initrd_open, METHOD_DIRECT);
    add_method(filesystem, "seek", initrd_seek, METHOD_DIRECT);
    add_method(filesystem, "makeDirectory", initrd_readonly, METHOD_DIRECT);
    add_method(filesystem, "exists", initrd_exists, METHOD_DIRECT);
    add_method(filesystem, "isReadOnly", initrd_is_read_only, METHOD_DIRECT);
    add_method(filesystem, "write", initrd_readonly, METHOD_DIRECT);
    add_method(filesystem, "spaceTotal", initrd_space_used, METHOD_DIRECT | METHOD_GETTER);
    // isDirectory
    add_method(filesystem, "rename", initrd_readonly, METHOD_DIRECT);
    // list
    // lastModified
    add_method(filesystem, "getLabel", initrd_get_label, METHOD_DIRECT | METHOD_GETTER);
    add_method(filesystem, "remove", initrd_readonly, METHOD_DIRECT);
    add_method(filesystem, "close", initrd_close, METHOD_DIRECT);
    add_method(filesystem, "size", initrd_size, METHOD_DIRECT);
    add_method(filesystem, "read", initrd_read, METHOD_DIRECT);
    add_method(filesystem, "setLabel", initrd_get_label, METHOD_DIRECT | METHOD_SETTER);
    add_component(filesystem);
}
