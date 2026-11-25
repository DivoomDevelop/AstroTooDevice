#include "divoom_pixel_format.h"
#include "divoom_lvgl9_compat.h"
#include <string.h>

uint32 divoom_pixel_format_bpp(lv_color_format_t cf)
{
    switch(cf) {
    case LV_COLOR_FORMAT_RGB565:
        return DIVOOM_APP_RGB565_BYTES_PER_PX;
    case LV_COLOR_FORMAT_RGB565A8:
        return DIVOOM_APP_RGB565A8_BYTES_PER_PX;
    case LV_COLOR_FORMAT_RGB888:
        return DIVOOM_APP_RGB888_BYTES_PER_PX;
    case LV_COLOR_FORMAT_XRGB8888:
    case LV_COLOR_FORMAT_ARGB8888:
    case LV_COLOR_FORMAT_RGBA8888:
        return DIVOOM_APP_ARGB8888_BYTES_PER_PX;
    default:
        return (uint32)lv_color_format_get_size(cf);
    }
}

uint32 divoom_pixel_format_size(uint32 w, uint32 h, lv_color_format_t cf)
{
    uint32 stride;

    if(w == 0U || h == 0U) {
        return 0U;
    }
    stride = divoom_pixel_format_stride(w, cf);
    if(cf == LV_COLOR_FORMAT_RGB565A8) {
        return stride * h + (stride / 2U) * h;
    }
    return stride * h;
}

uint32 divoom_pixel_format_stride(uint32 w, lv_color_format_t cf)
{
    if(w == 0U) {
        return 0U;
    }
    switch(cf) {
    case LV_COLOR_FORMAT_RGB565:
    case LV_COLOR_FORMAT_RGB565A8:
        return w * DIVOOM_APP_RGB565_BYTES_PER_PX;
    case LV_COLOR_FORMAT_RGB888:
        return w * DIVOOM_APP_RGB888_BYTES_PER_PX;
    case LV_COLOR_FORMAT_XRGB8888:
    case LV_COLOR_FORMAT_ARGB8888:
    case LV_COLOR_FORMAT_RGBA8888:
        return w * DIVOOM_APP_ARGB8888_BYTES_PER_PX;
    default:
        return w * divoom_pixel_format_bpp(cf);
    }
}

void divoom_lv_img_dsc_init_fmt(lv_img_dsc_t *dsc, uint32 w, uint32 h,
                                lv_color_format_t cf, const void *data)
{
    if(dsc == NULL) {
        return;
    }
    memset(dsc, 0, sizeof(*dsc));
    DIVOOM_LV_IMG_HEADER_INIT(&dsc->header);
    dsc->header.w = w;
    dsc->header.h = h;
    dsc->header.cf = cf;
#if !DIVOOM_LVGL9_IS_SIM
    dsc->header.stride = divoom_pixel_format_stride(w, cf);
#endif
    dsc->data_size = divoom_pixel_format_size(w, h, cf);
    dsc->data = data;
}

uint16 divoom_rgb888_to_rgb565(uint8 r, uint8 g, uint8 b)
{
    return (uint16)((((uint16)r & 0xF8U) << 8) |
                    (((uint16)g & 0xFCU) << 3) |
                    (((uint16)b) >> 3));
}

void divoom_store_rgb565_le(uint8 *dst, uint16 c)
{
    dst[0] = (uint8)(c & 0xFFU);
    dst[1] = (uint8)(c >> 8);
}

static uint16 divoom_pixel_format_load_rgb565_le(const uint8 *src)
{
    return (uint16)((uint16)src[0] | ((uint16)src[1] << 8));
}

/* 将网格色按 opa 预混到原图颜色中，避免透明网格露出页面背景造成突兀暗线。 */
static uint16 divoom_pixel_format_mix_rgb565(uint16 image_color,
                                             uint16 grid_color, uint8 opa)
{
    uint32 image_weight = 255U - (uint32)opa;
    uint32 grid_weight = (uint32)opa;
    uint32 red;
    uint32 green;
    uint32 blue;

    red = ((((uint32)image_color >> 11) & 0x1FU) * image_weight +
        (((uint32)grid_color >> 11) & 0x1FU) * grid_weight + 127U) / 255U;
    green = ((((uint32)image_color >> 5) & 0x3FU) * image_weight +
        (((uint32)grid_color >> 5) & 0x3FU) * grid_weight + 127U) / 255U;
    blue = (((uint32)image_color & 0x1FU) * image_weight +
        ((uint32)grid_color & 0x1FU) * grid_weight + 127U) / 255U;

    return (uint16)((red << 11) | (green << 5) | blue);
}

static void divoom_pixel_format_blend_grid_pixel(uint8 *buf, uint32 off,
                                                  uint16 grid_color, uint8 opa)
{
    uint8 *pixel = buf + off * DIVOOM_APP_RGB565_BYTES_PER_PX;
    uint16 image_color = divoom_pixel_format_load_rgb565_le(pixel);

    divoom_store_rgb565_le(pixel,
        divoom_pixel_format_mix_rgb565(image_color, grid_color, opa));
}

void divoom_rgb888_to_rgb565_buf(const uint8 *src_rgb, uint8 *dst_rgb565,
                                 uint32 px_count)
{
    uint32 i;
    if(src_rgb == NULL || dst_rgb565 == NULL) {
        return;
    }
    for(i = 0; i < px_count; i++) {
        uint16 c = divoom_rgb888_to_rgb565(src_rgb[i * 3U],
                                           src_rgb[i * 3U + 1U],
                                           src_rgb[i * 3U + 2U]);
        divoom_store_rgb565_le(&dst_rgb565[i * 2U], c);
    }
}

void divoom_rgb565_to_rgb888_buf(const uint8 *src_rgb565, uint8 *dst_rgb,
                                 uint32 px_count)
{
    uint32 i;
    if(src_rgb565 == NULL || dst_rgb == NULL) {
        return;
    }
    for(i = 0; i < px_count; i++) {
        uint16 c = (uint16)src_rgb565[i * 2U] |
                   ((uint16)src_rgb565[i * 2U + 1U] << 8);
        dst_rgb[i * 3U] = (uint8)(((c >> 11) & 0x1FU) << 3);
        dst_rgb[i * 3U + 1U] = (uint8)(((c >> 5) & 0x3FU) << 2);
        dst_rgb[i * 3U + 2U] = (uint8)((c & 0x1FU) << 3);
    }
}

void divoom_rgba8888_to_rgb565a8_buf(const uint8 *src_rgba,
                                     uint8 *dst_rgb565a8,
                                     uint32 px_count)
{
    uint32 i;
    uint8 *alpha;
    if(src_rgba == NULL || dst_rgb565a8 == NULL) {
        return;
    }
    alpha = dst_rgb565a8 + px_count * 2U;
    for(i = 0; i < px_count; i++) {
        uint16 c = divoom_rgb888_to_rgb565(src_rgba[i * 4U],
                                           src_rgba[i * 4U + 1U],
                                           src_rgba[i * 4U + 2U]);
        divoom_store_rgb565_le(&dst_rgb565a8[i * 2U], c);
        alpha[i] = src_rgba[i * 4U + 3U];
    }
}

void divoom_bgra8888_to_rgb565a8_buf(const uint8 *src_bgra,
                                     uint8 *dst_rgb565a8,
                                     uint32 px_count)
{
    uint32 i;
    uint8 *alpha;
    if(src_bgra == NULL || dst_rgb565a8 == NULL) {
        return;
    }
    alpha = dst_rgb565a8 + px_count * 2U;
    for(i = 0; i < px_count; i++) {
        uint16 c = divoom_rgb888_to_rgb565(src_bgra[i * 4U + 2U],
                                           src_bgra[i * 4U + 1U],
                                           src_bgra[i * 4U]);
        divoom_store_rgb565_le(&dst_rgb565a8[i * 2U], c);
        alpha[i] = src_bgra[i * 4U + 3U];
    }
}

void divoom_argb8888_to_rgb565a8_buf(const uint8 *src_argb,
                                     uint8 *dst_rgb565a8,
                                     uint32 px_count)
{
    uint32 i;
    uint8 *alpha;
    if(src_argb == NULL || dst_rgb565a8 == NULL) {
        return;
    }
    alpha = dst_rgb565a8 + px_count * 2U;
    for(i = 0; i < px_count; i++) {
        uint16 c = divoom_rgb888_to_rgb565(src_argb[i * 4U + 1U],
                                           src_argb[i * 4U + 2U],
                                           src_argb[i * 4U + 3U]);
        divoom_store_rgb565_le(&dst_rgb565a8[i * 2U], c);
        alpha[i] = src_argb[i * 4U];
    }
}

void divoom_xrgb8888_to_rgb565_buf(const uint8 *src_xrgb, uint8 *dst_rgb565,
                                   uint32 px_count)
{
    uint32 i;
    if(src_xrgb == NULL || dst_rgb565 == NULL) {
        return;
    }
    for(i = 0; i < px_count; i++) {
        uint16 c = divoom_rgb888_to_rgb565(src_xrgb[i * 4U + 2U],
                                           src_xrgb[i * 4U + 1U],
                                           src_xrgb[i * 4U]);
        divoom_store_rgb565_le(&dst_rgb565[i * 2U], c);
    }
}

static uint32 divoom_src_stride(const lv_img_dsc_t *src)
{
#if !DIVOOM_LVGL9_IS_SIM
    if(src != NULL && src->header.stride != 0U) {
        return src->header.stride;
    }
#endif
    return src != NULL ?
        divoom_pixel_format_stride((uint32)src->header.w,
                                   (lv_color_format_t)src->header.cf) : 0U;
}

static void divoom_read_lv_pixel(const lv_img_dsc_t *src, uint32 x, uint32 y,
                                 uint8 *r, uint8 *g, uint8 *b, uint8 *a)
{
    uint32 stride = divoom_src_stride(src);
    const uint8 *line = src->data + stride * y;

    *a = 0xFFU;
    switch((lv_color_format_t)src->header.cf) {
    case LV_COLOR_FORMAT_RGB565:
    {
        const uint8 *p = line + x * 2U;
        uint16 c = (uint16)p[0] | ((uint16)p[1] << 8);
        *r = (uint8)(((c >> 11) & 0x1FU) << 3);
        *g = (uint8)(((c >> 5) & 0x3FU) << 2);
        *b = (uint8)((c & 0x1FU) << 3);
        break;
    }
    case LV_COLOR_FORMAT_RGB565A8:
    {
        const uint8 *p = line + x * 2U;
        const uint8 *alpha = src->data + stride * src->header.h +
            (stride / 2U) * y;
        uint16 c = (uint16)p[0] | ((uint16)p[1] << 8);
        *r = (uint8)(((c >> 11) & 0x1FU) << 3);
        *g = (uint8)(((c >> 5) & 0x3FU) << 2);
        *b = (uint8)((c & 0x1FU) << 3);
        *a = alpha[x];
        break;
    }
    case LV_COLOR_FORMAT_RGB888:
    {
        const uint8 *p = line + x * 3U;
        *r = p[0];
        *g = p[1];
        *b = p[2];
        break;
    }
    case LV_COLOR_FORMAT_ARGB8888:
    {
        const uint8 *p = line + x * 4U;
        *b = p[0];
        *g = p[1];
        *r = p[2];
        *a = p[3];
        break;
    }
    case LV_COLOR_FORMAT_RGBA8888:
    {
        const uint8 *p = line + x * 4U;
        *r = p[0];
        *g = p[1];
        *b = p[2];
        *a = p[3];
        break;
    }
    case LV_COLOR_FORMAT_XRGB8888:
    default:
    {
        const uint8 *p = line + x * 4U;
        *b = p[0];
        *g = p[1];
        *r = p[2];
        break;
    }
    }
}

int divoom_lv_img_dsc_convert_buf(const lv_img_dsc_t *src, uint8 *dst,
                                  lv_color_format_t dst_cf)
{
    uint32 x;
    uint32 y;
    uint32 w;
    uint32 h;
    uint32 dst_stride;
    uint8 *dst_alpha = NULL;

    if(src == NULL || src->data == NULL || dst == NULL) {
        return 0;
    }
    if(dst_cf != LV_COLOR_FORMAT_RGB565 &&
        dst_cf != LV_COLOR_FORMAT_RGB565A8) {
        return 0;
    }

    w = (uint32)src->header.w;
    h = (uint32)src->header.h;
    dst_stride = divoom_pixel_format_stride(w, dst_cf);
    if(dst_cf == LV_COLOR_FORMAT_RGB565A8) {
        dst_alpha = dst + dst_stride * h;
    }

    for(y = 0; y < h; y++) {
        uint8 *dst_line = dst + dst_stride * y;
        for(x = 0; x < w; x++) {
            uint8 r;
            uint8 g;
            uint8 b;
            uint8 a;
            uint16 c;

            divoom_read_lv_pixel(src, x, y, &r, &g, &b, &a);
            c = divoom_rgb888_to_rgb565(r, g, b);
            dst_line[x * 2U] = (uint8)(c & 0xFFU);
            dst_line[x * 2U + 1U] = (uint8)(c >> 8);
            if(dst_alpha != NULL) {
                dst_alpha[(dst_stride / 2U) * y + x] = a;
            }
        }
    }
    return 1;
}

void divoom_rgb565a8_set_alpha(uint8 *buf, uint32 px_count, uint8 alpha)
{
    if(buf == NULL) {
        return;
    }
    memset(buf + px_count * 2U, alpha, px_count);
}

void divoom_rgb565a8_clear(uint8 *buf, uint32 px_count, uint16 rgb565,
                           uint8 alpha)
{
    uint32 i;
    if(buf == NULL) {
        return;
    }
    for(i = 0; i < px_count; i++) {
        divoom_store_rgb565_le(&buf[i * 2U], rgb565);
    }
    divoom_rgb565a8_set_alpha(buf, px_count, alpha);
}

void divoom_pixel_format_set_small_image_grid_opa(uint8 *buf, uint32 src_w,
                                                  uint32 src_h, uint32 dst_w,
                                                  uint32 dst_h,
                                                  lv_color_format_t cf,
                                                  uint8 opa)
{
    uint16 grid_color;
    uint32 line_sep_x;
    uint32 line_sep_y;
    uint32 x;
    uint32 y;
    uint32 off;

    if(buf == NULL || src_w != src_h ||
        (src_w != 16U && src_w != 32U) ||
        dst_w <= src_w || dst_h <= src_h ||
        (cf != LV_COLOR_FORMAT_RGB565 &&
        cf != LV_COLOR_FORMAT_RGB565A8))
    {
        return;
    }

    line_sep_x = dst_w / src_w;
    line_sep_y = dst_h / src_h;
    grid_color = divoom_rgb888_to_rgb565(0x80, 0x80, 0x80);

    for(y = line_sep_y; y < dst_h; y += line_sep_y)
    {
        off = y * dst_w;
        for(x = 0U; x < dst_w; x++)
        {
            divoom_pixel_format_blend_grid_pixel(buf, off, grid_color, opa);
            off++;
        }
    }

    for(x = line_sep_x; x < dst_w; x += line_sep_x)
    {
        off = x;
        for(y = 0U; y < dst_h; y++)
        {
            /* 横竖线交点只混合一次，确保整条网格均为同一透明度。 */
            if(y % line_sep_y != 0U)
            {
                divoom_pixel_format_blend_grid_pixel(buf, off, grid_color, opa);
            }
            off += dst_w;
        }
    }
}

int divoom_rgb565a8_to_lvfmt_nearest(const uint8 *src_rgb565a8, uint32 src_w,
                                     uint32 src_h, uint8 *dst_buf,
                                     uint32 dst_w, uint32 dst_h,
                                     lv_color_format_t dst_cf)
{
    const uint8 *src_alpha;
    uint32 src_px_count;

    if(src_rgb565a8 == NULL || dst_buf == NULL || src_w == 0U ||
        src_h == 0U || dst_w == 0U || dst_h == 0U)
    {
        return 0;
    }
    if(dst_cf != LV_COLOR_FORMAT_RGB565 &&
        dst_cf != LV_COLOR_FORMAT_RGB565A8)
    {
        return 0;
    }

    if(src_w == dst_w && src_h == dst_h && dst_cf == LV_COLOR_FORMAT_RGB565A8)
    {
        memcpy(dst_buf, src_rgb565a8,
            divoom_pixel_format_size(dst_w, dst_h, dst_cf));
        return 1;
    }

    src_px_count = src_w * src_h;
    src_alpha = src_rgb565a8 + src_px_count * DIVOOM_APP_RGB565_BYTES_PER_PX;

    for(uint32 y = 0; y < dst_h; y++)
    {
        uint32 sy = y * src_h / dst_h;
        for(uint32 x = 0; x < dst_w; x++)
        {
            uint32 sx = x * src_w / dst_w;
            uint32 src_dot = sy * src_w + sx;
            uint32 dst_dot = y * dst_w + x;

            dst_buf[dst_dot * 2U] = src_rgb565a8[src_dot * 2U];
            dst_buf[dst_dot * 2U + 1U] = src_rgb565a8[src_dot * 2U + 1U];
            if(dst_cf == LV_COLOR_FORMAT_RGB565A8)
            {
                dst_buf[dst_w * dst_h * 2U + dst_dot] = src_alpha[src_dot];
            }
        }
    }
    return 1;
}
