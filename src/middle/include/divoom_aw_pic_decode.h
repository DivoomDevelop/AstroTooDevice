#ifndef _divoom_aw_pic_decode_h_
#define _divoom_aw_pic_decode_h_

#include "divoom_type.h"
#include "divoom_lvgl9_compat.h"

/**
 * F101 Allwinner 硬件 JPEG/PNG 解码（jpegdecode.h / pngdecode.h）。
 * 仅真机编译；模拟器走 sim_rk_mpp_soft.c。
 */

uint8 *divoom_aw_decode_jpeg_rgb565_buf(char *buf, int buf_len, int *image_w, int *image_h);
uint8 *divoom_aw_decode_jpeg_rgb565_to_buf(char *buf, int buf_len, int *image_w, int *image_h,
    uint8 *out_buf, uint32 out_size);

#ifndef DIVOOM_SIMULATOR
/* 回调仅在持有图片解码锁期间借用 ABGR 帧；返回后即归还硬件缓冲。 */
typedef int divoom_aw_png_frame_callback(const uint8 *abgr, int width,
    int height, int stride_bytes, void *user_data);
int divoom_aw_png_decode_shared(const char *buf, int buf_len,
    int *width, int *height, divoom_aw_png_frame_callback *frame_callback,
    void *user_data);
#endif

uint8 *divoom_aw_decode_png_buf(const char *buf, int buf_len, int *width, int *height);
uint8 divoom_aw_parse_png_ihdr_info(const char *buf, int buf_len, int *width, int *height);
uint8 divoom_aw_parse_jpeg_sof_info(const char *buf, int buf_len, int *width, int *height);

typedef struct
{
    uint8 *out_buf;
    uint32 out_buf_size;
    uint8 out_buf_owner;
} divoom_aw_png_decode_buf_t;

void divoom_aw_png_decode_buf_release(divoom_aw_png_decode_buf_t *reuse);
uint8 *divoom_aw_decode_png_buf_to_lvfmt(const char *buf, int buf_len,
    int *width, int *height, uint8 *out_buf, uint32 out_size,
    lv_color_format_t cf);
uint8 *divoom_aw_decode_png_buf_to_lvfmt_reuse(const char *buf, int buf_len,
    int *width, int *height, divoom_aw_png_decode_buf_t *reuse,
    lv_color_format_t cf);

uint8 *divoom_aw_decode_png_file(const char *filename, int *width, int *height);
uint8 *divoom_aw_decode_png_file_to_lvfmt_reuse(const char *filename,
    int *width, int *height, divoom_aw_png_decode_buf_t *reuse,
    lv_color_format_t cf);
uint16 divoom_aw_rgb_to_rgb565(uint8 r, uint8 g, uint8 b);

#endif /* _divoom_aw_pic_decode_h_ */
