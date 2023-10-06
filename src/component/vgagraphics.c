#include <string.h>
#include "vgatext.h"
#include "gpu.h"
#include "io.h"
#include <malloc.h>
#include "multiboot.h"

struct vga_character {
    // bool wide;
    uint8_t pixels[16]; // Split into 2 chunks, the first 16 are for the first half of the character, the second 16 for the latter half
};


extern struct multiboot_header *mboot_ptr;

static struct vga_character* font;
static int vga_width;
static int vga_height;
static int vga_char_width;
static int vga_char_height;
static int vga_depth;
static uint32_t* vga_framebuffer;

static size_t hex_to_int(char hex) {
    if ('0' <= hex && hex <= '9') return hex - '0';
    switch (hex) {
        case 'a':
        case 'A':
            return 0xa;
        case 'b':
        case 'B':
            return 0xb;
        case 'c':
        case 'C':
            return 0xc;
        case 'd':
        case 'D':
            return 0xd;
        case 'e':
        case 'E':
            return 0xe;
        case 'f':
        case 'F':
            return 0xf;
    }
    return 0x0;
}

static bool load_single_character(char* line, size_t line_length) {
    size_t index = 0;
    size_t i = 0;
    while (line[i] != ':') {
        index <<= 4;
        index |= hex_to_int(line[i]);
        i += 1;
    }
    if (index >= 0x10000) return true;
    struct vga_character* character = font + index;
    size_t subindex = 0;
    // Skip over the ":" character
    i += 1;
    for (;i < line_length; i+=2, subindex += 1) {
        char hi = line[i];
        char lo = line[i+1];
        size_t value = hex_to_int(lo) | (hex_to_int(hi) << 4);
        if (subindex < 16)
            character->pixels[subindex] = value;
    }
    return false;
    // if (subindex > 16) {
    //     character->wide = true;
    // }
}

void vgagraphics_load_font(char* font_data, size_t font_data_size) {
    size_t start = 0;
    while (start < font_data_size) {
        size_t newline = start;
        for (int i = start; i < font_data_size; i++) {
            newline = i;
            if (font_data[i] == '\n') break;
        }
        size_t len = newline - start;
        if (load_single_character(font_data + start, len)) return;
        start = newline+1;
    }
}
static void rect(size_t px, size_t py, size_t w, size_t h, uint32_t color) {
    for (size_t y = py; y < py+h; y++) {
        size_t idx = y * vga_width + px;
        if (y < vga_height)
            for (size_t x = 0; x < w; x++) {
                if (x+px < vga_width)
                    vga_framebuffer[idx + x] = color;
            }
    }
}

static void setPixel(size_t px, size_t py, uint32_t foreground) {
    if (px < vga_width && py < vga_height) {
        size_t idx = py * vga_width + px;
        vga_framebuffer[idx] = foreground;
    }
}

static void set(int x, int y, uint32_t c, int foreground, int background) {
    uint16_t index = c & 0xffff;
    size_t px = x * 8;
    size_t py = y * 16;
    struct vga_character* character = font + index;
    // printf("'%c', %d, %d -> %d, %d\n", c, x, y, px, py);
    rect(px,py,8,16,background);
    for (size_t row = 0; row < 16; row++) {
        uint8_t rowPixels = character->pixels[row];
        for (size_t column = 0; column < 8; column++) {
            size_t reverse_column = 8 - column;
            bool isSet = (rowPixels & 1) == 1;
            if (isSet) {
                setPixel(px+reverse_column,py+row,foreground);
            }
            rowPixels >>= 1;
        }
    }
    // if (false /*character->wide*/) {
    //     rect(px,py,16,16,background);
    //     for (size_t row = 0; row < 16; row++) {
    //         uint8_t rowPixelsFirst = character->pixels[row*2];
    //         uint8_t rowPixelsSecond = character->pixels[row * 2 + 1];
    //         uint16_t rowPixels = (((uint16_t)rowPixelsFirst) << 8) | rowPixelsSecond;
    //         for (size_t column = 0; column < 16; column++) {
    //             size_t reverse_column = 16 - column;
    //             bool isSet = (rowPixels & 1) == 1;
    //             if (isSet) {
    //                 setPixel(px+reverse_column,py+row,foreground);
    //             }
    //             rowPixels >>= 1;
    //         }
    //     }
    // } else {
        
    // }
}

static void copy(int x, int y, int width, int height, int target_x, int target_y) {
    // Scale up to the font size
    x *= 8;
    y *= 16;
    width *= 8;
    height *= 16;
    target_x *= 8;
    target_y *= 16;
    if (target_y < y) {
        for (int i = 0; i < height; i++, y++, target_y++) {
            memmove(&vga_framebuffer[target_y * vga_width + target_x],&vga_framebuffer[y * vga_width + x], width * 4);
        }
    } else {
        y += height;
        target_y += height;
        for (int i = 0; i < height; i++) {
            memmove(&vga_framebuffer[--target_y * vga_width + target_x],&vga_framebuffer[--y * vga_width + x], width * 4);
        }
    }
}



static struct gpu *vgagraphics_gpu;

struct gpu *vgagraphics_init(void) {
    // We initialize the font storage in memory
    font = calloc(65536,sizeof(struct vga_character));
    vgagraphics_gpu = (struct gpu*)malloc(sizeof(struct gpu));
    vga_width = mboot_ptr->framebuffer_width;
    vga_height = mboot_ptr->framebuffer_height;
    vga_char_width = mboot_ptr->framebuffer_width >> 3;
    vga_char_height = mboot_ptr->framebuffer_height >> 4;
    vga_depth = mboot_ptr->framebuffer_bpp;
    vga_framebuffer = mboot_ptr->framebuffer_addr;
    *vgagraphics_gpu = (struct gpu){
        .width = vga_char_width,
        .height = vga_char_height,
        .depth = vga_depth,
        .palette_size = 0,
        .palette = NULL,
        .set = set,
        .copy = copy
    };
    gpu_init(vgagraphics_gpu);
    return vgagraphics_gpu;
}

