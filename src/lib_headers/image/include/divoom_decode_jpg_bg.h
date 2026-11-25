#ifndef _divoom_decode_jpg_bg_h_
#define _divoom_decode_jpg_bg_h_
#include "lvgl.h"

void *divoom_decode_jpg_bg_init(char *file_name, int *width, int *high);
void *divoom_decode_jpg_bg_init_scaled(char *file_name, int max_w, int max_h,
	int *width, int *high);
void *divoom_decode_jpg_bg_init_scaled_lvfmt(char *file_name, int max_w, int max_h,
	int *width, int *high, lv_color_format_t cf);
uint8 *divoom_decode_jpg_bg_get_rgba_buf(void *handle);
uint8 *divoom_decode_jpg_bg_get_lvfmt_buf(void *handle);
int divoom_decode_jpg_bg_decode_to_lvfmt(void *handle, uint8 *dst, uint32 dst_size, lv_color_format_t cf);
uint8 *divoom_decode_jpg_bg_steal_rgba_buf(void *handle);
uint8 *divoom_decode_jpg_bg_steal_lvfmt_buf(void *handle);
void *divoom_decode_jpg_bg_exit(void *handle);
void *divoom_decode_jpg_bg_init_with_type(char *file_name, int resource_type, int *width, int *high);

#endif
