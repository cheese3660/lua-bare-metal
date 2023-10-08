#pragma once
#include <stddef.h>
struct gpu *vgagraphics_init(void);
void vgagraphics_load_font(char* font_data, size_t font_data_size);