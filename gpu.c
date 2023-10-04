#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include "gpu.h"
#include "uuid.h"
#include "componentlib.h"

struct stored_character {
    uint32_t character;
    int foreground;
    int background;
};

static int return_true(lua_State *L, const char *address, void *data) {
    lua_pushboolean(L, true);
    lua_pushboolean(L, true);
    return 2;
}

static int return_false(lua_State *L, const char *address, void *data) {
    lua_pushboolean(L, true);
    lua_pushboolean(L, false);
    return 2;
}

static int throw_unsupported(lua_State *L, const char *address, void *data) {
    lua_pushboolean(L, false);
    lua_pushliteral(L, "unsupported");
    return 2;
}

static int screen_get_aspect_ratio(lua_State *L, const char *address, void *data) {
    lua_pushboolean(L, true);
    lua_pushnumber(L, 1);
    lua_pushnumber(L, 1);
    return 3;
}

static int screen_get_keyboards(lua_State *L, const char *address, void *data) {
    lua_pushboolean(L, true);
    lua_newtable(L);
    return 2;
}

static void create_screen(struct gpu *gpu) {
    gpu->screen_address = new_uuid();

    struct component *screen = new_component("screen", gpu->screen_address, gpu);
    add_method(screen, "isOn", return_true, METHOD_DIRECT | METHOD_GETTER);
    add_method(screen, "turnOn", return_false, METHOD_DIRECT | METHOD_SETTER);
    add_method(screen, "turnOff", return_false, METHOD_DIRECT | METHOD_SETTER);
    add_method(screen, "getAspectRatio", screen_get_aspect_ratio, METHOD_DIRECT | METHOD_GETTER);
    add_method(screen, "getKeyboards", screen_get_keyboards, METHOD_DIRECT | METHOD_GETTER);
    add_method(screen, "setPrecise", return_true, METHOD_DIRECT | METHOD_SETTER);
    add_method(screen, "isPrecise", return_true, METHOD_DIRECT | METHOD_GETTER);
    add_method(screen, "setTouchModeInverted", return_false, METHOD_DIRECT | METHOD_SETTER);
    add_method(screen, "isTouchModeInverted", return_false, METHOD_DIRECT | METHOD_GETTER);
    add_component(screen);
}

static int gpu_bind(lua_State *L, const char *address, void *data) {
    lua_pushboolean(L, true);
    lua_pushboolean(L, false);
    lua_pushliteral(L, "unsupported");
    return 3;
}

static int gpu_get_screen(lua_State *L, const char *address, struct gpu *gpu) {
    lua_pushboolean(L, true);
    lua_pushstring(L, gpu->screen_address);
    return 2;
}

static int gpu_get_background(lua_State *L, const char *address, struct gpu *gpu) {
    lua_pushboolean(L, true);
    lua_pushnumber(L, gpu->background);
    lua_pushboolean(L, gpu->palette_size > 0);
    return 3;
}

static int gpu_get_foreground(lua_State *L, const char *address, struct gpu *gpu) {
    lua_pushboolean(L, true);
    lua_pushnumber(L, gpu->foreground);
    lua_pushboolean(L, gpu->palette_size > 0);
    return 3;
}

static int find_closest_color(struct gpu *gpu, int color) {
    int target_r = (color >> 16) & 0xff;
    int target_g = (color >> 8) & 0xff;
    int target_b = color & 0xff;

    int closest = 0;
    int closest_distance = INT_MAX;

    int *in_palette = gpu->palette;
    for (int i = 0; i < gpu->palette_size; i++, *in_palette++) {
        int r = (*in_palette >> 16) & 0xff;
        int g = (*in_palette >> 8) & 0xff;
        int b = *in_palette & 0xff;

        int distance = abs(target_r - r) + abs(target_g - g) + abs(target_b - b);

        if (distance < closest_distance) {
            closest_distance = distance;
            closest = i;
        }

        if (distance == 0)
            break;
    }

    return closest;
}

static int gpu_set_background(lua_State *L, const char *address, struct gpu *gpu) {
    int color = luaL_checknumber(L, 3);
    bool is_palette_index = lua_isboolean(L, 4) ? lua_toboolean(L, 4) : false;

    if (gpu->palette_size == 0) {
        gpu->background = color;
        lua_pushboolean(L, true);
        lua_pushnumber(L, gpu->background);
        return 2;
    }

    if (!is_palette_index)
        color = find_closest_color(gpu, color);

    if (color >= gpu->palette_size) {
        lua_pushboolean(L, false);
        lua_pushliteral(L, "palette index is outside bounds of palette");
        return 2;
    }

    gpu->background = color;
    lua_pushboolean(L, true);
    lua_pushnumber(L, gpu->palette[gpu->background]);
    lua_pushnumber(L, gpu->background);
    return 3;
}

static int gpu_set_foreground(lua_State *L, const char *address, struct gpu *gpu) {
    int color = luaL_checknumber(L, 3);
    bool is_palette_index = lua_isboolean(L, 4) ? lua_toboolean(L, 4) : false;

    if (gpu->palette_size == 0) {
        gpu->foreground = color;
        lua_pushboolean(L, true);
        lua_pushnumber(L, gpu->foreground);
        return 2;
    }

    if (!is_palette_index)
        color = find_closest_color(gpu, color);

    if (color >= gpu->palette_size) {
        lua_pushboolean(L, false);
        lua_pushliteral(L, "palette index is outside bounds of palette");
        return 2;
    }

    gpu->foreground = color;
    lua_pushboolean(L, true);
    lua_pushnumber(L, gpu->palette[gpu->foreground]);
    lua_pushnumber(L, gpu->foreground);
    return 3;
}

static int gpu_get_palette_color(lua_State *L, const char *address, struct gpu *gpu) {
    if (gpu->palette_size == 0) {
        lua_pushboolean(L, false);
        lua_pushliteral(L, "no palette exists for this GPU");
        return 2;
    }

    int color = luaL_checknumber(L, 3);

    if (color >= gpu->palette_size) {
        lua_pushboolean(L, false);
        lua_pushliteral(L, "palette index is outside bounds of palette");
        return 2;
    }

    lua_pushboolean(L, true);
    lua_pushnumber(L, gpu->palette[color]);
    return 2;
}

static int gpu_get_depth(lua_State *L, const char *address, struct gpu *gpu) {
    lua_pushboolean(L, true);
    lua_pushnumber(L, gpu->depth);
    return 2;
}

static int gpu_get_resolution(lua_State *L, const char *address, struct gpu *gpu) {
    lua_pushboolean(L, true);
    lua_pushnumber(L, gpu->width);
    lua_pushnumber(L, gpu->height);
    return 3;
}

static int gpu_get(lua_State *L, const char *address, struct gpu *gpu) {
    int x = luaL_checknumber(L, 3) - 1;
    int y = luaL_checknumber(L, 4) - 1;

    if (x < 0 || y < 0 || x >= gpu->width || y >= gpu->height) {
        lua_pushboolean(L, false);
        lua_pushliteral(L, "position out of bounds");
        return 2;
    }

    struct stored_character *c = &gpu->stored[y * gpu->width + x];

    lua_pushfstring(L, "%U", c->character);

    if (gpu->palette_size > 0) {
        lua_pushnumber(L, gpu->palette[c->foreground]);
        lua_pushnumber(L, gpu->palette[c->background]);
        lua_pushnumber(L, c->foreground);
        lua_pushnumber(L, c->background);
    } else {
        lua_pushnumber(L, c->foreground);
        lua_pushnumber(L, c->background);
        lua_pushnil(L);
        lua_pushnil(L);
    }

    return 5;
}

#define MAXUNICODE	0x10FFFFu
#define MAXUTF		0x7FFFFFFFu

/* from lua/lutf8lib.c */
/*
 * Decode one UTF-8 sequence, returning NULL if byte sequence is
 * invalid.  The array 'limits' stores the minimum value for each
 * sequence length, to check for overlong representations. Its first
 * entry forces an error for non-ascii bytes with no continuation
 * bytes (count == 0).
 */
static const char *utf8_decode (const char *s, uint32_t *val, int strict) {
    static const uint32_t limits[] = {~(uint32_t)0, 0x80, 0x800, 0x10000u, 0x200000u, 0x4000000u};
    unsigned int c = (unsigned char)s[0];
    uint32_t res = 0;  /* final result */
    if (c < 0x80)  /* ascii? */
        res = c;
    else {
        int count = 0;  /* to count number of continuation bytes */
        for (; c & 0x40; c <<= 1) {  /* while it needs continuation bytes... */
            unsigned int cc = (unsigned char)s[++count];  /* read next byte */
            if ((cc & 0xC0) != 0x80)  /* not a continuation byte? */
                return NULL;  /* invalid byte sequence */
            res = (res << 6) | (cc & 0x3F);  /* add lower 6 bits from cont. byte */
        }
        res |= ((uint32_t)(c & 0x7F) << (count * 5));  /* add first byte */
        if (count > 5 || res > MAXUTF || res < limits[count])
            return NULL;  /* invalid byte sequence */
        s += count;  /* skip continuation bytes read */
    }
    if (strict) {
        /* check for invalid code points; too large or surrogates */
        if (res > MAXUNICODE || (0xD800u <= res && res <= 0xDFFFu))
            return NULL;
    }
    if (val) *val = res;
    return s + 1;  /* +1 to include first byte */
}

static int gpu_set(lua_State *L, const char *address, struct gpu *gpu) {
    int x = luaL_checknumber(L, 3) - 1;
    int y = luaL_checknumber(L, 4) - 1;
    const char *string = luaL_checkstring(L, 5);
    bool vertical = lua_isboolean(L, 6) ? lua_toboolean(L, 6) : false;
    uint32_t c;

    if (x >= gpu->width || y >= gpu->height) {
        lua_pushboolean(L, true);
        lua_pushboolean(L, false);
        return 2;
    }

    while (*string) {
        if ((string = utf8_decode(string, &c, true)) == NULL) {
            lua_pushboolean(L, false);
            lua_pushliteral(L, "invalid UTF-8 code");
            return 2;
        }

        if (x >= 0 && y >= 0 && x < gpu->width && y < gpu->height) {
            gpu->set(x, y, c, gpu->foreground, gpu->background);
            struct stored_character *stored = &gpu->stored[y * gpu->width + x];
            stored->character = c;
            stored->foreground = gpu->foreground;
            stored->background = gpu->background;
        }

        if (vertical) {
            y++;
            if (y >= gpu->height)
                break;
        } else {
            x++;
            if (x >= gpu->width)
                break;
        }
    }

    lua_pushboolean(L, true);
    lua_pushboolean(L, true);
    return 2;
}

static int gpu_copy(lua_State *L, const char *address, struct gpu *gpu) {
    int x = luaL_checknumber(L, 3) - 1;
    int y = luaL_checknumber(L, 4) - 1;
    int width = luaL_checknumber(L, 5);
    int height = luaL_checknumber(L, 6);
    int target_x = x + luaL_checknumber(L, 7);
    int target_y = y + luaL_checknumber(L, 8);

    /* all of this is undefined behavior so if it doesn't work properly that's the documentation's fault, not mine :3 */
    if (width <= 0 || height <= 0 || x >= gpu->width || y >= gpu->height || target_x >= gpu->width || target_y >= gpu->height || (target_x == x && target_y == y)) {
        lua_pushboolean(L, true);
        lua_pushboolean(L, false);
        return 2;
    }

    if (x < 0) {
        width += x;
        target_x += x;
        x = 0;
    }

    if (target_x < 0) {
        width += target_x;
        x += target_x;
        target_x = 0;
    }

    if (y < 0) {
        height += y;
        target_y += y;
        y = 0;
    }

    if (target_y < 0) {
        height += target_y;
        y += target_y;
        target_y = 0;
    }

    if (x + width >= gpu->width)
        width -= gpu->width - (x + width);

    if (target_x + width >= gpu->width)
        width -= gpu->width - (target_x + width);

    if (y + height >= gpu->height)
        height -= gpu->height - (y + height);

    if (target_y + height >= gpu->height)
        height -= gpu->height - (target_y + height);

    bool has_copy = gpu->copy != NULL;

    if (has_copy)
        gpu->copy(x, y, width, height, target_x, target_y);

    if (target_y < y) // copy downwards
        for (int i = 0; i < height; i++, y++, target_y++)
            memmove(&gpu->stored[target_y * gpu->width + target_x], &gpu->stored[y * gpu->width + x], sizeof(struct stored_character) * width);
    else { // copy upwards
        y += height;
        target_y += height;
        for (int i = 0; i < height; i++)
            memmove(&gpu->stored[--target_y * gpu->width + target_x], &gpu->stored[--y * gpu->width + x], sizeof(struct stored_character) * width);
    }

    if (!has_copy) // redraw target area
        for (int copy_y = target_y; copy_y < target_y + height; copy_y++)
            for (int copy_x = target_x; copy_x < target_x + width; copy_x++) {
                struct stored_character *c = &gpu->stored[copy_y * gpu->width + copy_x];
                gpu->set(copy_x, copy_y, c->character, c->foreground, c->background);
            }

    lua_pushboolean(L, true);
    lua_pushboolean(L, true);
    return 2;
}

static int fill(struct gpu *gpu, int x0, int y0, int x1, int y1, uint32_t c) {
    if (x0 < 0)
        x0 = 0;

    if (y0 < 0)
        y0 = 0;

    if (x1 > gpu->width)
        x1 = gpu->width;

    if (y1 > gpu->height)
        y1 = gpu->height;

    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            gpu->set(x, y, c, gpu->foreground, gpu->background);
            struct stored_character *stored = &gpu->stored[y * gpu->width + x];
            stored->character = c;
            stored->foreground = gpu->foreground;
            stored->background = gpu->background;
        }
}

static int gpu_fill(lua_State *L, const char *address, struct gpu *gpu) {
    int x = luaL_checknumber(L, 3);
    int y = luaL_checknumber(L, 4);
    int width = luaL_checknumber(L, 5);
    int height = luaL_checknumber(L, 6);
    const char *string = luaL_checkstring(L, 7);
    uint32_t c;

    if (!*string || width <= 0 || height <= 0) {
        lua_pushboolean(L, true);
        lua_pushboolean(L, false);
        return 2;
    }

    if (utf8_decode(string, &c, true) == NULL) {
        lua_pushboolean(L, false);
        lua_pushliteral(L, "invalid UTF-8 code");
        return 2;
    }

    fill(gpu, x - 1, y - 1, x - 1 + width, y - 1 + height, c);

    lua_pushboolean(L, true);
    lua_pushboolean(L, true);
    return 2;
}

void gpu_init(struct gpu *gpu) {
    create_screen(gpu);

    struct component *gpu_component = new_component("gpu", new_uuid(), gpu);
    add_method(gpu_component, "bind", gpu_bind, METHOD_DIRECT);
    add_method(gpu_component, "getScreen", gpu_get_screen, METHOD_DIRECT | METHOD_GETTER);
    add_method(gpu_component, "getBackground", gpu_get_background, METHOD_DIRECT | METHOD_GETTER);
    add_method(gpu_component, "getForeground", gpu_get_foreground, METHOD_DIRECT | METHOD_GETTER);
    add_method(gpu_component, "setBackground", gpu_set_background, METHOD_DIRECT | METHOD_SETTER);
    add_method(gpu_component, "setForeground", gpu_set_foreground, METHOD_DIRECT | METHOD_SETTER);
    add_method(gpu_component, "getPaletteColor", gpu_get_palette_color, METHOD_DIRECT | METHOD_GETTER);
    add_method(gpu_component, "setPaletteColor", throw_unsupported, METHOD_DIRECT | METHOD_SETTER);
    add_method(gpu_component, "maxDepth", gpu_get_depth, METHOD_DIRECT);
    add_method(gpu_component, "getDepth", gpu_get_depth, METHOD_DIRECT | METHOD_GETTER);
    add_method(gpu_component, "setDepth", throw_unsupported, METHOD_DIRECT | METHOD_SETTER);
    add_method(gpu_component, "maxResolution", gpu_get_resolution, METHOD_DIRECT);
    add_method(gpu_component, "getResolution", gpu_get_resolution, METHOD_DIRECT | METHOD_GETTER);
    add_method(gpu_component, "setResolution", return_false, METHOD_DIRECT | METHOD_SETTER);
    add_method(gpu_component, "getViewport", gpu_get_resolution, METHOD_DIRECT | METHOD_GETTER);
    add_method(gpu_component, "setViewport", return_false, METHOD_DIRECT | METHOD_SETTER);
    add_method(gpu_component, "get", gpu_get, METHOD_DIRECT | METHOD_GETTER);
    add_method(gpu_component, "set", gpu_set, METHOD_DIRECT | METHOD_SETTER);
    add_method(gpu_component, "copy", gpu_copy, METHOD_DIRECT);
    add_method(gpu_component, "fill", gpu_fill, METHOD_DIRECT);
    add_component(gpu_component);

    if (gpu->palette_size == 0) {
        gpu->foreground = 0xffffff;
        gpu->background = 0x000000;
    } else {
        gpu->foreground = find_closest_color(gpu, 0xffffff);
        gpu->background = find_closest_color(gpu, 0x000000);
    }

    gpu->stored = malloc(sizeof(struct stored_character) * gpu->width * gpu->height);
    assert(gpu->stored != NULL);

    fill(gpu, 0, 0, gpu->width, gpu->height, ' ');
}

void gpu_error_message(struct gpu *gpu, const char *message) {
    printf("%s\n", message);

    if (gpu->palette_size == 0) {
        gpu->foreground = 0xffffff;
        gpu->background = 0x0000ff;
    } else {
        gpu->foreground = find_closest_color(gpu, 0xffffff);
        gpu->background = find_closest_color(gpu, 0x0000ff);
    }

    fill(gpu, 0, 0, gpu->width, gpu->height, ' ');

    int x = 0;
    int y = 0;

    while (*message) {
        char c = *message++;
        if (c >= ' ') {
            gpu->set(x, y, c, gpu->foreground, gpu->background);
            struct stored_character *stored = &gpu->stored[y * gpu->width + x];
            stored->character = c;
            stored->foreground = gpu->foreground;
            stored->background = gpu->background;
        }

        x++;
        if (x >= gpu->width || c == '\n') {
            x = 0;
            y++;
            if (y >= gpu->height)
                break;
        }
    }
}
