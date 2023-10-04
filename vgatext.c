#include <string.h>
#include "vgatext.h"
#include "gpu.h"
#include "io.h"

uint16_t *video_memory = (uint16_t *) 0xb8000;

static void set(int x, int y, uint32_t c, int foreground, int background) {
    if (c > 256)
        c = '?';

    video_memory[y * 80 + x] = ((background & 0xf) << 12) | ((foreground & 0xf) << 8) | (c & 0xff);
}

static void copy(int x, int y, int width, int height, int target_x, int target_y) {
    if (target_y < y) // copy downwards
        for (int i = 0; i < height; i++, y++, target_y++)
            memmove(&video_memory[target_y * 80 + target_x], &video_memory[y * 80 + x], width * 2);
    else { // copy upwards
        y += height;
        target_y += height;
        for (int i = 0; i < height; i++)
            memmove(&video_memory[--target_y * 80 + target_x], &video_memory[--y * 80 + x], width * 2);
    }
}

static int palette[16] = {
    0x000000, 0x0000aa, 0x00aa00, 0x00aaaa, 0xaa0000, 0xaa00aa, 0xaa5500, 0xaaaaaa,
    0x555555, 0x5555ff, 0x55ff55, 0x55ffff, 0xff5555, 0xff55ff, 0xffff55, 0xffffff
};

static struct gpu vgatext_gpu = {
    .width = 80,
    .height = 25,
    .depth = 4,
    .palette_size = 16,
    .palette = &palette,
    .set = set,
    .copy = copy
};

struct gpu *vgatext_init(void) {
    // disable cursor
    outb(0x3d4, 0x0a);
	outb(0x3d5, 0x20);
    gpu_init(&vgatext_gpu);
    return &vgatext_gpu;
}
