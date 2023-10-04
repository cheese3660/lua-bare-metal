#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct gpu {
    /* === required stuff === */
    /* the width of the screen in characters */
    int width;
    /* the height of the screen in characters */
    int height;
    /* the color depth of the screen in bits */
    uint8_t depth;

    /* the size of the screen's color palette, or 0 if it's full color */
    size_t palette_size;
    /* the screen's color palette, as an array of 24-bit rgb values */
    int *palette;

    /* sets a single character on-screen. x and y are 0 based */
    void (*set)(int x, int y, uint32_t c, int foreground, int background);
    /* copies part of the screen somewhere else. x and y are 0 based, tx and ty are absolute instead of relative */
    void (*copy)(int x, int y, int width, int height, int target_x, int target_y);

    /* === internal stuff === */
    const char *screen_address;
    int background;
    int foreground;
    struct stored_character *stored;
};

void gpu_init(struct gpu *gpu);
