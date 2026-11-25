#ifndef _DIVOOM_PIXEL_FORMAT_H_
#define _DIVOOM_PIXEL_FORMAT_H_

#include "divoom_type.h"
#include "divoom_lvgl9_compat.h"

#define DIVOOM_APP_RGB565_BYTES_PER_PX   2U
#define DIVOOM_APP_RGB565A8_BYTES_PER_PX 3U
#define DIVOOM_APP_RGB888_BYTES_PER_PX   3U
#define DIVOOM_APP_ARGB8888_BYTES_PER_PX 4U

uint32 divoom_pixel_format_bpp(lv_color_format_t cf);
uint32 divoom_pixel_format_stride(uint32 w, lv_color_format_t cf);
uint32 divoom_pixel_format_size(uint32 w, uint32 h, lv_color_format_t cf);

void divoom_lv_img_dsc_init_fmt(lv_img_dsc_t *dsc, uint32 w, uint32 h,
                                lv_color_format_t cf, const void *data);

uint16 divoom_rgb888_to_rgb565(uint8 r, uint8 g, uint8 b);
void divoom_store_rgb565_le(uint8 *dst, uint16 c);

void divoom_rgb888_to_rgb565_buf(const uint8 *src_rgb, uint8 *dst_rgb565,
                                 uint32 px_count);
void divoom_rgb565_to_rgb888_buf(const uint8 *src_rgb565, uint8 *dst_rgb,
                                 uint32 px_count);
void divoom_rgba8888_to_rgb565a8_buf(const uint8 *src_rgba,
                                     uint8 *dst_rgb565a8,
                                     uint32 px_count);
void divoom_bgra8888_to_rgb565a8_buf(const uint8 *src_bgra,
                                     uint8 *dst_rgb565a8,
                                     uint32 px_count);
void divoom_argb8888_to_rgb565a8_buf(const uint8 *src_argb,
                                     uint8 *dst_rgb565a8,
                                     uint32 px_count);
void divoom_xrgb8888_to_rgb565_buf(const uint8 *src_xrgb,
                                   uint8 *dst_rgb565,
                                   uint32 px_count);

int divoom_lv_img_dsc_convert_buf(const lv_img_dsc_t *src, uint8 *dst,
                                  lv_color_format_t dst_cf);

void divoom_rgb565a8_set_alpha(uint8 *buf, uint32 px_count, uint8 alpha);
void divoom_rgb565a8_clear(uint8 *buf, uint32 px_count, uint16 rgb565,
                           uint8 alpha);
/*
 * 为 16x16/32x32 像素画的放大结果添加网格线。
 * opa 表示网格色叠加比例；RGB565A8 保留原 Alpha 平面，让原图自然透过网格。
 */
void divoom_pixel_format_set_small_image_grid_opa(uint8 *buf, uint32 src_w,
                                                  uint32 src_h, uint32 dst_w,
                                                  uint32 dst_h,
                                                  lv_color_format_t cf,
                                                  uint8 opa);
int divoom_rgb565a8_to_lvfmt_nearest(const uint8 *src_rgb565a8, uint32 src_w,
                                     uint32 src_h, uint8 *dst_buf,
                                     uint32 dst_w, uint32 dst_h,
                                     lv_color_format_t dst_cf);

#endif /* _DIVOOM_PIXEL_FORMAT_H_ */
