#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include "initrd.h"
#include "api/component.h"
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

static int initrd_space_used(lua_State *L, struct initrd_data *data, int arguments_start) {
    lua_pushnumber(L, data->end - data->start);
    return 1;
}

static int initrd_open(lua_State *L, struct initrd_data *data, int arguments_start) {
    const char *path = luaL_checkstring(L, arguments_start);
    const char *mode = lua_isstring(L, arguments_start + 1) ? lua_tostring(L, arguments_start + 1) : "r";

    if (!*path || !*mode)
        return luaL_error(L, "bad argument");

    if (strstr(mode, "r") == NULL || strstr(mode, "w") != NULL)
        return luaL_error(L, "read-only filesystem");

    struct tar_iterator *iter = open_tar(data->start, data->end);
    const char *file_data;
    size_t file_size;
    if (!tar_find(iter, path, TAR_NORMAL_FILE, &file_data, &file_size))
        return luaL_error(L, "file not found");

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

    lua_pushnumber(L, open_file->handle_num);
    return 1;
}

static int initrd_readonly(lua_State *L, struct initrd_data *data, int arguments_start) {
    return luaL_error(L, "read-only filesystem");
}

static struct open_file *find_open_file(struct initrd_data *data, int handle) {
    for (struct open_file *open_file = data->open_files; open_file != NULL; open_file = open_file->next)
        if (open_file->handle_num == handle)
            return open_file;

    return NULL;
}

static int initrd_seek(lua_State *L, struct initrd_data *data, int arguments_start) {
    int handle = luaL_checknumber(L, arguments_start);
    const char *whence = luaL_checkstring(L, arguments_start + 1);
    int offset = luaL_checknumber(L, arguments_start + 2);

    struct open_file *open_file = find_open_file(data, handle);

    if (open_file == NULL)
        return luaL_error(L, "invalid handle");

    if (!strcmp(whence, "cur")) {
        open_file->read_pos += offset;
    } else if (!strcmp(whence, "set")) {
        open_file->read_pos = offset;
    } else if (!strcmp(whence, "end")) {
        open_file->read_pos = open_file->size + offset;
    }

    lua_pushnumber(L, open_file->read_pos);
    return 1;
}

static int initrd_exists(lua_State *L, struct initrd_data *data, int arguments_start) {
    const char *path = luaL_checkstring(L, arguments_start);

    if (!*path) {
        lua_pushboolean(L, false);
        return 1;
    }

    struct tar_iterator *iter = open_tar(data->start, data->end);
    const char *file_data;
    size_t file_size;

    lua_pushboolean(L, tar_find(iter, path, 0, &file_data, &file_size));
    return 1;
}

static int initrd_is_read_only(lua_State *L, void *data, int arguments_start) {
    lua_pushboolean(L, true);
    return 1;
}

static int initrd_get_label(lua_State *L, struct initrd_data *data, int arguments_start) {
    lua_pushstring(L, data->name);
    return 1;
}

static int initrd_size(lua_State *L, struct initrd_data *data, int arguments_start) {
    const char *path = luaL_checkstring(L, arguments_start);

    if (!*path) {
        lua_pushboolean(L, false);
        return 1;
    }

    struct tar_iterator *iter = open_tar(data->start, data->end);
    const char *file_data;
    size_t file_size;
    if (!tar_find(iter, path, TAR_NORMAL_FILE, &file_data, &file_size))
        return luaL_error(L, "file not found");

    lua_pushnumber(L, file_size);
    return 1;
}

static int initrd_close(lua_State *L, struct initrd_data *data, int arguments_start) {
    int handle = luaL_checknumber(L, arguments_start);

    struct open_file *open_file = data->open_files, *last = NULL;

    for (; open_file != NULL; last = open_file, open_file = open_file->next)
        if (open_file->handle_num == handle) {
            if (last == NULL)
                data->open_files = open_file->next;
            else
                last->next = open_file->next;

            free(open_file);

            return 0;
        }

    return luaL_error(L, "invalid handle");
}

static int initrd_read(lua_State *L, struct initrd_data *data, int arguments_start) {
    int handle = luaL_checknumber(L, arguments_start);

    // lua is on crack
    uint64_t count2 = luaL_checknumber(L, arguments_start);
    size_t count = count2 >= UINTPTR_MAX ? UINTPTR_MAX : count;

    struct open_file *open_file = find_open_file(data, handle);

    if (open_file == NULL)
        return luaL_error(L, "invalid handle");

    if (open_file->read_pos >= open_file->size) {
        lua_pushnil(L);
        return 1;
    }

    if (count > open_file->size)
        count = open_file->size;

    if (open_file->read_pos + count >= open_file->size)
        count -= open_file->size - (open_file->read_pos + count);
    
    if (count == 0) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushlstring(L, open_file->start + open_file->read_pos, count);

    open_file->read_pos += count;

    return 1;
}

static int initrd_is_directory(lua_State *L, struct initrd_data *data, int arguments_start) {
    const char *path = luaL_checkstring(L, arguments_start);

    struct tar_iterator *iter = open_tar(data->start, data->end);
    const char *file_data;
    size_t file_size;
    if (!tar_find(iter, path, 0, &file_data, &file_size))
        return luaL_error(L, "file not found");

    struct tar_header *header = file_data - 512; // hehe

    lua_pushboolean(L, header->kind == TAR_DIRECTORY);
    return 1;
}

static int initrd_last_modified(lua_State *L, struct initrd_data *data, int arguments_start) {
    const char *path = luaL_checkstring(L, arguments_start);

    if (!*path) {
        lua_pushboolean(L, false);
        return 1;
    }

    struct tar_iterator *iter = open_tar(data->start, data->end);
    const char *file_data;
    size_t file_size;
    if (!tar_find(iter, path, 0, &file_data, &file_size))
        return luaL_error(L, "file not found");

    struct tar_header *header = file_data - 512; // hehe

    lua_pushnumber(L, oct2bin(&header->mod_time, 11));
    return 1;
}

static int initrd_list(lua_State *L, struct initrd_data *data, int arguments_start) {
    const char name[256];
    const char *path = luaL_checkstring(L, arguments_start);

    if (*path == '/')
        path++;

    size_t len = strlen(path);

    if (path[len - 1] != '/') {
        const char *old_path = path;
        path = malloc(++len);
        assert(path != NULL);
        sprintf(path, "%s/", old_path);
    }

    lua_newtable(L);

    struct tar_iterator *iter = open_tar(data->start, data->end);
    const char *file_data;
    size_t file_size;

    if (!tar_find(iter, path, TAR_DIRECTORY, &file_data, &file_size))
        return luaL_error(L, "directory not found");

    for (int i = 1;; i++) {
        struct tar_header *header;
        const char *name_ptr = &name;

        if (!next_file(iter, &header, &file_data, &file_size))
            break;

        header->name[99] = 0;
        header->filename_prefix[154] = 0;
        sprintf(name, "%s%s", header->name, header->filename_prefix);

        if (*name_ptr == '/')
            name_ptr++;

        if (strstr(name_ptr, path) != name_ptr)
            continue;

        name_ptr += len;
        size_t name_len = strlen(name_ptr);

        const char *slash_pos = strchr(name_ptr, '/');
        if (slash_pos != name_ptr + name_len - 1 && slash_pos != NULL)
            continue; /* ignore anything in subdirectories */

        lua_pushnumber(L, i);
        lua_pushstring(L, name_ptr);
        lua_rawset(L, -3);
    }

    return 1;
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
    add_method(filesystem, "isDirectory", initrd_is_directory, METHOD_DIRECT);
    add_method(filesystem, "rename", initrd_readonly, METHOD_DIRECT);
    add_method(filesystem, "list", initrd_list, METHOD_DIRECT);
    add_method(filesystem, "lastModified", initrd_last_modified, METHOD_DIRECT);
    add_method(filesystem, "getLabel", initrd_get_label, METHOD_DIRECT | METHOD_GETTER);
    add_method(filesystem, "remove", initrd_readonly, METHOD_DIRECT);
    add_method(filesystem, "close", initrd_close, METHOD_DIRECT);
    add_method(filesystem, "size", initrd_size, METHOD_DIRECT);
    add_method(filesystem, "read", initrd_read, METHOD_DIRECT);
    add_method(filesystem, "setLabel", initrd_get_label, METHOD_DIRECT | METHOD_SETTER);
    add_component(filesystem);
}
