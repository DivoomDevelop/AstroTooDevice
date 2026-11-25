#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <turbojpeg.h>
#include "divoom.h"
#include "divoom_menu_control.h"
#include "divoom_pixel_format.h"
#if !defined(DIVOOM_SIMULATOR) && defined(CONFIG_DRIVER_G2D_200)
#ifdef G2D_FORMAT_ARGB8888
#undef G2D_FORMAT_ARGB8888
#endif
#ifdef G2D_FORMAT_ABGR8888
#undef G2D_FORMAT_ABGR8888
#endif
#include "g2d_200/g2d_driver.h"
#include <hal_cache.h>
extern int sunxi_g2d_open(void);
extern int sunxi_g2d_control(int cmd, void *arg);
extern void *sunxifb_mem_get_phyaddr(void *data);
extern void sunxifb_mem_flush_cache(void *data, size_t size);
extern void sunxifb_mem_invalidate_cache(void *data, size_t size);
#endif
#ifndef DIVOOM_SIMULATOR
#include "middle/include/divoom_aw_pic_decode.h"
#else
/* 仿真无全志硬件 PNG 解码器：改用 LVGL 自带 lodepng 软解（与真机 .bak_port 同逻辑）。
 * lodepng 在本工程用 lv_malloc 分配（见 LvglPlatform/lvgl/src/libs/png/lodepng.c），
 * 返回的 RGBA 缓冲可用 lv_mem_free(=lv_free) 释放，与上层缓存释放契约一致。
 * 这里前置声明以避免 lvgl libs 头文件包含路径脆弱。 */
extern unsigned lodepng_decode32(unsigned char **out, unsigned *w, unsigned *h,
    const unsigned char *in, size_t insize);
extern const char *lodepng_error_text(unsigned code);
#endif

#ifndef G2D_FORMAT_RGB888
#define G2D_FORMAT_RGB888 8
#endif
#ifndef G2D_FORMAT_BGR888
#define G2D_FORMAT_BGR888 9
#endif
#ifndef G2D_FORMAT_RGB565
#define G2D_FORMAT_RGB565 10
#endif
#ifndef G2D_FORMAT_BGR565
#define G2D_FORMAT_BGR565 11
#endif
#ifndef G2D_FORMAT_ARGB4444
#define G2D_FORMAT_ARGB4444 12
#endif
#ifndef G2D_FORMAT_ABGR4444
#define G2D_FORMAT_ABGR4444 13
#endif
#ifndef G2D_FORMAT_RGBA4444
#define G2D_FORMAT_RGBA4444 14
#endif
#ifndef G2D_FORMAT_BGRA4444
#define G2D_FORMAT_BGRA4444 15
#endif
#ifndef G2D_FORMAT_ARGB1555
#define G2D_FORMAT_ARGB1555 16
#endif
#ifndef G2D_FORMAT_ABGR1555
#define G2D_FORMAT_ABGR1555 17
#endif
#ifndef G2D_FORMAT_RGBA5551
#define G2D_FORMAT_RGBA5551 18
#endif
#ifndef G2D_FORMAT_BGRA5551
#define G2D_FORMAT_BGRA5551 19
#endif


int divoom_rk_mpp_convert_color_type(int src_type)
{
    switch(src_type)
    {
        case RK_FORMAT_BGRA_8888:
            return G2D_FORMAT_ABGR8888;
            
        case RK_FORMAT_RGB_888:
            return G2D_FORMAT_RGB888;
            
        case RK_FORMAT_RGBA_8888:
            return G2D_FORMAT_ARGB8888;
            
        case RK_FORMAT_RGB_565:
            return G2D_FORMAT_RGB565;

        default:
            return G2D_FORMAT_ABGR8888;
    }
}
 int divoom_rk_mpp_get_rgb_size_type(int src_type, int width, int high)
 {
     if(width <= 0 || high <= 0)
     {
         return 0;
     }
     switch(src_type)
     {
         case RK_FORMAT_BGRA_8888:
             return width * high *4;
             
         case RK_FORMAT_RGB_888:
             return  width * high *3;
             
         case RK_FORMAT_RGBA_8888:
             return  width * high *4;
             
         case RK_FORMAT_RGB_565:
             return  width * high *2;
 
         default:
             return  width * high *4;
     }
 }

static int divoom_rk_mpp_g2d_format_bpp(int fmt)
{
    switch(fmt)
    {
        case G2D_FORMAT_RGB565:
        case G2D_FORMAT_BGR565:
        case G2D_FORMAT_ARGB4444:
        case G2D_FORMAT_ABGR4444:
        case G2D_FORMAT_RGBA4444:
        case G2D_FORMAT_BGRA4444:
        case G2D_FORMAT_ARGB1555:
        case G2D_FORMAT_ABGR1555:
        case G2D_FORMAT_RGBA5551:
        case G2D_FORMAT_BGRA5551:
            return 2;

        case G2D_FORMAT_RGB888:
        case G2D_FORMAT_BGR888:
            return 3;

        default:
            return 4;
    }
}

static size_t divoom_rk_mpp_g2d_image_size(int fmt, int width, int high)
{
    if(width <= 0 || high <= 0)
    {
        return 0;
    }
    size_t bpp = (size_t)divoom_rk_mpp_g2d_format_bpp(fmt);
    if((size_t)width > SIZE_MAX / (size_t)high / bpp)
    {
        return 0;
    }
    return (size_t)width * (size_t)high * bpp;
}

static int divoom_rk_mpp_lv_cf_to_g2d(lv_color_format_t cf)
{
    switch(cf) {
        case LV_COLOR_FORMAT_RGB565:
            return G2D_FORMAT_RGB565;
        case LV_COLOR_FORMAT_RGB888:
            return G2D_FORMAT_RGB888;
        case LV_COLOR_FORMAT_ARGB8888:
        case LV_COLOR_FORMAT_XRGB8888:
        default:
            return G2D_FORMAT_ARGB8888;
    }
}

static void divoom_rk_mpp_read_pixel(const uint8 *src, int fmt,
    uint8 *r, uint8 *g, uint8 *b, uint8 *a)
{
    uint16 c;

    *a = 0xFF;
    switch(fmt) {
        case G2D_FORMAT_RGB565:
        case G2D_FORMAT_BGR565:
            c = (uint16)src[0] | ((uint16)src[1] << 8);
            *r = (uint8)(((c >> 11) & 0x1F) << 3);
            *g = (uint8)(((c >> 5) & 0x3F) << 2);
            *b = (uint8)((c & 0x1F) << 3);
            break;
        case G2D_FORMAT_RGB888:
            *r = src[0];
            *g = src[1];
            *b = src[2];
            break;
        case G2D_FORMAT_BGR888:
            *r = src[2];
            *g = src[1];
            *b = src[0];
            break;
        case G2D_FORMAT_ABGR8888:
            *r = src[2];
            *g = src[1];
            *b = src[0];
            *a = src[3];
            break;
        case G2D_FORMAT_ARGB8888:
        default:
            *r = src[0];
            *g = src[1];
            *b = src[2];
            *a = src[3];
            break;
    }
}

static void divoom_rk_mpp_write_pixel(uint8 *dst, int fmt,
    uint8 r, uint8 g, uint8 b, uint8 a)
{
    uint16 c;

    switch(fmt) {
        case G2D_FORMAT_RGB565:
        case G2D_FORMAT_BGR565:
            c = divoom_rgb888_to_rgb565(r, g, b);
            dst[0] = (uint8)(c & 0xFFU);
            dst[1] = (uint8)(c >> 8);
            break;
        case G2D_FORMAT_RGB888:
            dst[0] = r;
            dst[1] = g;
            dst[2] = b;
            break;
        case G2D_FORMAT_BGR888:
            dst[0] = b;
            dst[1] = g;
            dst[2] = r;
            break;
        case G2D_FORMAT_ABGR8888:
            dst[0] = b;
            dst[1] = g;
            dst[2] = r;
            dst[3] = a;
            break;
        case G2D_FORMAT_ARGB8888:
        default:
            dst[0] = r;
            dst[1] = g;
            dst[2] = b;
            dst[3] = a;
            break;
    }
}

static void divoom_rk_mpp_cpu_scale_convert(const uint8 *src, int src_w,
    int src_h, int src_fmt, int src_x, int src_y, int src_crop_w,
    int src_crop_h, uint8 *dst, int dst_w, int dst_h, int dst_fmt,
    int dst_x, int dst_y, int dst_rect_w, int dst_rect_h)
{
    int row;
    int col;
    const int src_bpp = divoom_rk_mpp_g2d_format_bpp(src_fmt);
    const int dst_bpp = divoom_rk_mpp_g2d_format_bpp(dst_fmt);

    if(src == NULL || dst == NULL || src_bpp <= 0 || dst_bpp <= 0)
    {
        return;
    }

    for(row = 0; row < dst_rect_h; row++)
    {
        int sy = src_y + (int)((int64_t)row * src_crop_h / dst_rect_h);
        if(sy >= src_y + src_crop_h)
        {
            sy = src_y + src_crop_h - 1;
        }
        for(col = 0; col < dst_rect_w; col++)
        {
            int sx = src_x + (int)((int64_t)col * src_crop_w / dst_rect_w);
            int dx = dst_x + col;
            int dy = dst_y + row;
            uint8 r, g, b, a;
            const uint8 *s;
            uint8 *d;

            if(sx >= src_x + src_crop_w)
            {
                sx = src_x + src_crop_w - 1;
            }
            s = src + ((size_t)sy * (size_t)src_w + (size_t)sx) * (size_t)src_bpp;
            d = dst + ((size_t)dy * (size_t)dst_w + (size_t)dx) * (size_t)dst_bpp;
            divoom_rk_mpp_read_pixel(s, src_fmt, &r, &g, &b, &a);
            divoom_rk_mpp_write_pixel(d, dst_fmt, r, g, b, a);
        }
    }
}

static void divoom_rk_mpp_cpu_scale_to_lvfmt(const uint8 *src, int src_w,
    int src_h, int src_fmt, int src_x, int src_y, int src_crop_w,
    int src_crop_h, uint8 *dst, int dst_w, int dst_h,
    lv_color_format_t dst_cf, int dst_x, int dst_y, int dst_rect_w,
    int dst_rect_h)
{
    int row;
    int src_bpp = divoom_rk_mpp_g2d_format_bpp(src_fmt);
    uint8 *alpha = NULL;
    int prev_sy = -1;
    int x_step;
    uint32_t x_rem;

    if(src == NULL || dst == NULL || src_bpp <= 0 ||
        dst_rect_w <= 0 || dst_rect_h <= 0) {
        return;
    }
    if(dst_cf != LV_COLOR_FORMAT_RGB565 && dst_cf != LV_COLOR_FORMAT_RGB565A8) {
        return;
    }
    if(dst_cf == LV_COLOR_FORMAT_RGB565A8) {
        alpha = dst + (size_t)dst_w * (size_t)dst_h *
            DIVOOM_APP_RGB565_BYTES_PER_PX;
    }
    /* Exact floor(col * src_crop_w / dst_rect_w), without per-pixel division. */
    x_step = src_crop_w / dst_rect_w;
    x_rem = (uint32_t)(src_crop_w % dst_rect_w);

    for(row = 0; row < dst_rect_h; row++) {
        int sy = src_y + (int)((int64_t)row * src_crop_h / dst_rect_h);
        size_t offset = (size_t)(dst_y + row) * (size_t)dst_w + (size_t)dst_x;
        uint8 *d = dst + offset * DIVOOM_APP_RGB565_BYTES_PER_PX;
        uint8 *a_row = alpha ? alpha + offset : NULL;
        const uint8 *s_row = src + ((size_t)sy * (size_t)src_w +
            (size_t)src_x) * (size_t)src_bpp;
        int sx = 0;
        uint32_t x_err = 0;
        int col;

        /* Upscaling repeats source rows: reuse the completed destination row. */
        if(sy == prev_sy) {
            memcpy(d, d - (size_t)dst_w * DIVOOM_APP_RGB565_BYTES_PER_PX,
                (size_t)dst_rect_w * DIVOOM_APP_RGB565_BYTES_PER_PX);
            if(a_row != NULL) {
                memcpy(a_row, a_row - dst_w, (size_t)dst_rect_w);
            }
            continue;
        }
        prev_sy = sy;
        if(src_fmt == G2D_FORMAT_RGB565) {
            /* Preserve RGB565 bytes; no RGB888 round trip or alignment requirement. */
            if(src_crop_w == dst_rect_w) {
                memcpy(d, s_row, (size_t)dst_rect_w * DIVOOM_APP_RGB565_BYTES_PER_PX);
            }
            else {
                for(col = 0; col < dst_rect_w; col++) {
                    const uint8 *s = s_row + (size_t)sx * DIVOOM_APP_RGB565_BYTES_PER_PX;
                    d[0] = s[0];
                    d[1] = s[1];
                    d += DIVOOM_APP_RGB565_BYTES_PER_PX;
                    sx += x_step;
                    x_err += x_rem;
                    if(x_err >= (uint32_t)dst_rect_w) {
                        x_err -= (uint32_t)dst_rect_w;
                        sx++;
                    }
                }
            }
            if(a_row != NULL) {
                memset(a_row, 0xFF, (size_t)dst_rect_w);
            }
        }
        else {
            for(col = 0; col < dst_rect_w; col++) {
                const uint8 *s = s_row + (size_t)sx * (size_t)src_bpp;
                uint8 r, g, b, a;
                uint16 c;

                divoom_rk_mpp_read_pixel(s, src_fmt, &r, &g, &b, &a);
                c = divoom_rgb888_to_rgb565(r, g, b);
                d[0] = (uint8)(c & 0xFFU);
                d[1] = (uint8)(c >> 8);
                d += DIVOOM_APP_RGB565_BYTES_PER_PX;
                if(a_row != NULL) {
                    a_row[col] = a;
                }
                sx += x_step;
                x_err += x_rem;
                if(x_err >= (uint32_t)dst_rect_w) {
                    x_err -= (uint32_t)dst_rect_w;
                    sx++;
                }
            }
        }
    }
}

static int divoom_rk_mpp_rect_valid(int x, int y, int w, int h, int max_w, int max_h)
{
    return x >= 0 && y >= 0 && w > 0 && h > 0 &&
        max_w > 0 && max_h > 0 &&
        x <= max_w - w && y <= max_h - h;
}

#if !defined(DIVOOM_SIMULATOR) && defined(CONFIG_DRIVER_G2D_200)
/*
 * 业务层格式按内存字节顺序描述（BGRA/RGBA），而 G2D_200 的 32 位枚举按
 * 小端机器上的字值位序命名。提交给硬件前必须交换 ARGB/ABGR，否则红蓝通道互换。
 */
static int divoom_rk_mpp_g2d_get_driver_format(int fmt)
{
    switch(fmt)
    {
        case G2D_FORMAT_ARGB8888:
        {
            return G2D_FORMAT_ABGR8888;
        }
        case G2D_FORMAT_ABGR8888:
        {
            return G2D_FORMAT_ARGB8888;
        }
        default:
        {
            return fmt;
        }
    }
}

static int divoom_rk_mpp_g2d_should_use_cpu_scale(int src_w, int src_h,
    int src_crop_w, int src_crop_h, int dst_rect_w, int dst_rect_h)
{
    const int src_is_small_square =
        src_w == src_h &&
        (src_w == 16 || src_w == 32 || src_w == 64 ||
            src_w == 128 || src_w == 256);
    const int dst_is_target_square =
        dst_rect_w == dst_rect_h &&
        (dst_rect_w == 256 || dst_rect_w == 480);
    const int is_upscale =
        dst_rect_w > src_crop_w || dst_rect_h > src_crop_h;

    return src_is_small_square && dst_is_target_square && is_upscale;
}
#endif

static void divoom_rk_mpp_cpu_scale_copy(const uint8 *src, int src_w, int src_h,
    int src_fmt, int src_x, int src_y, int src_crop_w, int src_crop_h,
    uint8 *dst, int dst_w, int dst_h, int dst_fmt,
    int dst_x, int dst_y, int dst_rect_w, int dst_rect_h)
{
    int row;
    int col;
    const int src_bpp = divoom_rk_mpp_g2d_format_bpp(src_fmt);
    const int dst_bpp = divoom_rk_mpp_g2d_format_bpp(dst_fmt);
    const int copy_bpp = (src_bpp < dst_bpp) ? src_bpp : dst_bpp;

    if(src == NULL || dst == NULL || src_bpp <= 0 || dst_bpp <= 0)
    {
        return;
    }

    for(row = 0; row < dst_rect_h; row++)
    {
        int sy = src_y + (int)((int64_t)row * src_crop_h / dst_rect_h);
        if(sy >= src_y + src_crop_h)
        {
            sy = src_y + src_crop_h - 1;
        }
        for(col = 0; col < dst_rect_w; col++)
        {
            int sx = src_x + (int)((int64_t)col * src_crop_w / dst_rect_w);
            uint8 *d;
            const uint8 *s;

            if(sx >= src_x + src_crop_w)
            {
                sx = src_x + src_crop_w - 1;
            }
            s = src + ((size_t)sy * (size_t)src_w + (size_t)sx) * (size_t)src_bpp;
            d = dst + ((size_t)(dst_y + row) * (size_t)dst_w +
                    (size_t)(dst_x + col)) * (size_t)dst_bpp;
            memcpy(d, s, (size_t)copy_bpp);
            if(dst_bpp == 4 && src_bpp < 4)
            {
                d[3] = 0xff;
            }
        }
    }
}

static void divoom_rk_mpp_zero_buffer(uint8 *buf, int fmt, int width, int high)
{
    size_t size = divoom_rk_mpp_g2d_image_size(fmt, width, high);

    if(buf != NULL && size > 0)
    {
        memset(buf, 0, size);
    }
}

static int divoom_rk_mpp_g2d_blit_scale(uint8 *src, int src_w, int src_h,
    int src_fmt, int src_x, int src_y, int src_crop_w, int src_crop_h,
    uint8 *dst, int dst_w, int dst_h, int dst_fmt,
    int dst_x, int dst_y, int dst_rect_w, int dst_rect_h)
{
#if !defined(DIVOOM_SIMULATOR) && defined(CONFIG_DRIVER_G2D_200)
    static int g_g2d_opened;
    g2d_blt_h info;
    void *src_phy;
    void *dst_phy;
    size_t src_size;
    size_t dst_size;

    if(src == NULL || dst == NULL ||
        !divoom_rk_mpp_rect_valid(src_x, src_y, src_crop_w, src_crop_h, src_w, src_h) ||
        !divoom_rk_mpp_rect_valid(dst_x, dst_y, dst_rect_w, dst_rect_h, dst_w, dst_h))
    {
        return 0;
    }

    if(divoom_rk_mpp_g2d_should_use_cpu_scale(src_w, src_h,
        src_crop_w, src_crop_h, dst_rect_w, dst_rect_h))
    {
        return 0;
    }

    src_size = divoom_rk_mpp_g2d_image_size(src_fmt, src_w, src_h);
    dst_size = divoom_rk_mpp_g2d_image_size(dst_fmt, dst_w, dst_h);
    if(src_size == 0 || dst_size == 0)
    {
        return 0;
    }

    /* C907 invalidates complete 64-byte lines. A shared boundary line
     * could discard a neighbouring heap header changed while DMA runs. */
    if(((uintptr_t)dst & 63U) != 0 || (dst_size & 63U) != 0)
    {
        return 0; /* The caller already supplies a CPU fallback. */
    }

    if(!g_g2d_opened)
    {
        if(sunxi_g2d_open() < 0)
        {
            return 0;
        }
        g_g2d_opened = 1;
    }

    src_phy = sunxifb_mem_get_phyaddr(src);
    dst_phy = sunxifb_mem_get_phyaddr(dst);
    if(src_phy == NULL || dst_phy == NULL)
    {
        return 0;
    }

    /* The source is read-only for DMA; invalidation could discard a dirty
     * neighbour on a shared boundary line. Only write it back. */
    hal_dcache_clean((unsigned long)((uintptr_t)src & ~(uintptr_t)63U),
        src_size + ((uintptr_t)src & 63U));
    sunxifb_mem_flush_cache(dst, dst_size);

    memset(&info, 0, sizeof(info));
    info.flag_h = G2D_BLT_NONE_H;
    info.src_image_h.format = (g2d_fmt_enh)
        divoom_rk_mpp_g2d_get_driver_format(src_fmt);
    info.src_image_h.width = src_w;
    info.src_image_h.height = src_h;
    info.src_image_h.clip_rect.x = src_x;
    info.src_image_h.clip_rect.y = src_y;
    info.src_image_h.clip_rect.w = src_crop_w;
    info.src_image_h.clip_rect.h = src_crop_h;
    info.src_image_h.mode = G2D_PIXEL_ALPHA;
    info.src_image_h.alpha = 255;
    info.src_image_h.color = 0xee8899;
    info.src_image_h.laddr[0] = (uintptr_t)src_phy;
    info.src_image_h.use_phy_addr = 1;
    info.src_image_h.bbuff = 1;

    info.dst_image_h.format = (g2d_fmt_enh)
        divoom_rk_mpp_g2d_get_driver_format(dst_fmt);
    info.dst_image_h.width = dst_w;
    info.dst_image_h.height = dst_h;
    info.dst_image_h.clip_rect.x = dst_x;
    info.dst_image_h.clip_rect.y = dst_y;
    info.dst_image_h.clip_rect.w = dst_rect_w;
    info.dst_image_h.clip_rect.h = dst_rect_h;
    info.dst_image_h.mode = G2D_PIXEL_ALPHA;
    info.dst_image_h.alpha = 255;
    info.dst_image_h.color = 0xee8899;
    info.dst_image_h.laddr[0] = (uintptr_t)dst_phy;
    info.dst_image_h.use_phy_addr = 1;
    info.dst_image_h.bbuff = 1;

    if(sunxi_g2d_control(G2D_CMD_BITBLT_H, &info) < 0)
    {
        return 0;
    }

    sunxifb_mem_invalidate_cache(dst, dst_size);
    return 1;
#else
    (void)src;
    (void)src_w;
    (void)src_h;
    (void)src_fmt;
    (void)src_x;
    (void)src_y;
    (void)src_crop_w;
    (void)src_crop_h;
    (void)dst;
    (void)dst_w;
    (void)dst_h;
    (void)dst_fmt;
    (void)dst_x;
    (void)dst_y;
    (void)dst_rect_w;
    (void)dst_rect_h;
    return 0;
#endif
}
 


/*********************************************************************
  Function:        divoom_rk_mpp_stretch_image
  Description:     对图像进行拉伸
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   dest_w
                   dest_h
  Return:          0为成功，其它失败原因
**********************************************************************/


void *divoom_rk_stretch_lock = NULL;
int divoom_rk_mpp_stretch_image_direct(uint8 *src_rgb, int src_w, int src_h, int src_type,uint8 *dest_rgb, int dest_w, int dest_h, int dest_type)
{
    int src_fmt = divoom_rk_mpp_convert_color_type(src_type);
    int dst_fmt = divoom_rk_mpp_convert_color_type(dest_type);

    if(src_rgb == NULL || dest_rgb == NULL || src_w <= 0 || src_h <= 0 ||
        dest_w <= 0 || dest_h <= 0)
    {
        return 0;
    }

    if(src_w == dest_w && src_h == dest_h && src_fmt == dst_fmt)
    {
        size_t size = divoom_rk_mpp_g2d_image_size(src_fmt, src_w, src_h);
        if(size == 0)
        {
            return 0;
        }
        memcpy(dest_rgb, src_rgb, size);
        return 1;
    }

    divoom_rk_mpp_zero_buffer(dest_rgb, dst_fmt, dest_w, dest_h);
    if(divoom_rk_mpp_g2d_blit_scale(src_rgb, src_w, src_h, src_fmt,
            0, 0, src_w, src_h, dest_rgb, dest_w, dest_h, dst_fmt,
            0, 0, dest_w, dest_h))
    {
        return 1;
    }

    divoom_rk_mpp_cpu_scale_convert(src_rgb, src_w, src_h, src_fmt,
        0, 0, src_w, src_h, dest_rgb, dest_w, dest_h, dst_fmt,
        0, 0, dest_w, dest_h);
    return 1;
}

/*********************************************************************
  Function:        divoom_rk_mpp_crop_stretch_image_direct
  Description:     对图像进行拉伸
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   dest_w
                   dest_h
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_crop_stretch_image_direct(int src_type, uint8 *src_rgb, int src_w, int src_h, int src_start_x, int src_start_y,
                                            int src_size_x, int src_size_y,uint8 *dest_rgb, int dest_w, int dest_h, 
                                            int dest_start_x, int dest_start_y,int dest_size_x, int dest_size_y)
{
    if(src_rgb == NULL || dest_rgb == NULL ||
        !divoom_rk_mpp_rect_valid(src_start_x, src_start_y, src_size_x, src_size_y, src_w, src_h) ||
        !divoom_rk_mpp_rect_valid(dest_start_x, dest_start_y, dest_size_x, dest_size_y, dest_w, dest_h))
    {
        return 0;
    }

    if(divoom_rk_mpp_g2d_blit_scale(src_rgb, src_w, src_h, src_type,
            src_start_x, src_start_y, src_size_x, src_size_y,
            dest_rgb, dest_w, dest_h, G2D_FORMAT_ARGB8888,
            dest_start_x, dest_start_y, dest_size_x, dest_size_y))
    {
        return 1;
    }

    divoom_rk_mpp_cpu_scale_convert(src_rgb, src_w, src_h, src_type,
        src_start_x, src_start_y, src_size_x, src_size_y,
        dest_rgb, dest_w, dest_h, G2D_FORMAT_ARGB8888,
        dest_start_x, dest_start_y, dest_size_x, dest_size_y);
    return 1;
}

int divoom_rk_mpp_crop_stretch_image_lvfmt(int src_type, uint8 *src_rgb,
                                           int src_w, int src_h,
                                           int src_start_x, int src_start_y,
                                           int src_size_x, int src_size_y,
                                           uint8 *dest_rgb, int dest_w,
                                           int dest_h, int dest_start_x,
                                           int dest_start_y, int dest_size_x,
                                           int dest_size_y,
                                           lv_color_format_t dest_cf)
{
    int dst_fmt = divoom_rk_mpp_lv_cf_to_g2d(dest_cf);

    if(src_rgb == NULL || dest_rgb == NULL ||
        !divoom_rk_mpp_rect_valid(src_start_x, src_start_y, src_size_x, src_size_y, src_w, src_h) ||
        !divoom_rk_mpp_rect_valid(dest_start_x, dest_start_y, dest_size_x, dest_size_y, dest_w, dest_h))
    {
        return 0;
    }

    if(dest_cf == LV_COLOR_FORMAT_RGB565A8) {
        divoom_rgb565a8_clear(dest_rgb, (uint32)dest_w * (uint32)dest_h, 0, 0);
        divoom_rk_mpp_cpu_scale_to_lvfmt(src_rgb, src_w, src_h, src_type,
            src_start_x, src_start_y, src_size_x, src_size_y,
            dest_rgb, dest_w, dest_h, dest_cf,
            dest_start_x, dest_start_y, dest_size_x, dest_size_y);
        return 1;
    }

    divoom_rk_mpp_zero_buffer(dest_rgb, dst_fmt, dest_w, dest_h);
    if(divoom_rk_mpp_g2d_blit_scale(src_rgb, src_w, src_h, src_type,
            src_start_x, src_start_y, src_size_x, src_size_y,
            dest_rgb, dest_w, dest_h, dst_fmt,
            dest_start_x, dest_start_y, dest_size_x, dest_size_y))
    {
        return 1;
    }

    if(dest_cf == LV_COLOR_FORMAT_RGB565) {
        divoom_rk_mpp_cpu_scale_to_lvfmt(src_rgb, src_w, src_h, src_type,
            src_start_x, src_start_y, src_size_x, src_size_y,
            dest_rgb, dest_w, dest_h, dest_cf,
            dest_start_x, dest_start_y, dest_size_x, dest_size_y);
    }
    else {
        divoom_rk_mpp_cpu_scale_convert(src_rgb, src_w, src_h, src_type,
            src_start_x, src_start_y, src_size_x, src_size_y,
            dest_rgb, dest_w, dest_h, dst_fmt,
            dest_start_x, dest_start_y, dest_size_x, dest_size_y);
    }
    return 1;
}

/*********************************************************************
  Function:        divoom_rk_mpp_copy_stretch_image_direct
  Description:     对图像进行拷贝部分图像
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   dest_w
                   dest_h
                   dest_start_x
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_copy_stretch_image_direct(unsigned char* src_buffer, int src_width, int src_height,
                                                unsigned char* dst_buffer, int dst_width, int dst_height, int x)
{
     // BGRA格式每个像素4字节
    const int pixel_size = 4;
	//divoom_print("%d, %d, %d, %d\n", src_width, src_height, dst_width, dst_height);
    // 逐行拷贝
    for (int row = 0; row < src_height; row++) {
        // 计算源和目标的行起始位置
        const unsigned char* src_row = src_buffer + row * src_width * pixel_size;
        unsigned char* dst_row = dst_buffer + (row * dst_width + x) * pixel_size;
        
        // 拷贝一行数据
        memcpy(dst_row, src_row, src_width * pixel_size);
    }
    #if 0
	struct mixer_para *info;
	int g2d_fd, in_size, out_size;
	int ion_fd;
	struct ion_info src_ion;
	struct ion_info dst_ion;
	int src_fd = 0, dst_fd = 0;
	unsigned long arg[6];

    divoom_print("src: %d, %d, %d, %d, %d\n", src_w, src_h, dest_w,  dest_h,  dest_start_x);
    info = divoom_system_malloc(sizeof(struct mixer_para));
    memset(info, 0, sizeof(struct mixer_para));
    ion_fd = ion_open();
    if(ion_fd <= 0)
    {
		printf("ion_open device fail!\n");
        divoom_system_free(info);
        return -1;
    }
	
    in_size =  src_w * src_h * 4;
    out_size = dest_w * dest_h * 4;
    
    
	if((g2d_fd = open("/dev/g2d",O_RDWR)) <= 0) {
		divoom_print("open g2d device fail!\n");
        close(ion_fd);
		close(g2d_fd);
        divoom_system_free(info);
		return -1;
	}
    
    dst_fd = ion_memory_request(ion_fd, &dst_ion, out_size);
    
    src_fd = ion_memory_request(ion_fd, &src_ion, in_size);
    
    info->flag_h = G2D_BLT_NONE_H;
    info->op_flag = OP_BITBLT;
    info->src_image_h.format = G2D_FORMAT_ARGB8888;
    info->src_image_h.width = src_w;
    info->src_image_h.height = src_h;
    info->src_image_h.clip_rect.x = 0;
    info->src_image_h.clip_rect.y = 0;
    info->src_image_h.clip_rect.w = src_w;
    info->src_image_h.clip_rect.h = src_h;
    info->src_image_h.color = 0;
    info->src_image_h.mode = G2D_PIXEL_ALPHA;
    info->src_image_h.alpha = 255;
    info->src_image_h.align[0] = 0;
    info->src_image_h.align[1] = 0;
    info->src_image_h.align[2] = 0;

    info->dst_image_h.format = G2D_FORMAT_ARGB8888;
    info->dst_image_h.width = dest_w;
    info->dst_image_h.height = dest_h;
    info->dst_image_h.clip_rect.x = dest_start_x;
    info->dst_image_h.clip_rect.y = 0;
    info->dst_image_h.clip_rect.w = src_w;
    info->dst_image_h.clip_rect.h = src_h;
    info->dst_image_h.color = 0;
    info->dst_image_h.mode = G2D_PIXEL_ALPHA;
    info->dst_image_h.alpha = 255;
    info->dst_image_h.align[0] = 0;
    info->dst_image_h.align[1] = 0;
    info->dst_image_h.align[2] = 0;
    
    memcpy(dst_ion.virt_addr, dest_rgb, out_size);
    ion_flush_cache(dst_fd);
    info->dst_image_h.fd = dst_fd;

    memcpy(src_ion.virt_addr, src_rgb, in_size);
    ion_flush_cache(src_fd);
    info->src_image_h.fd = src_fd;
    
    arg[0] =(unsigned long)info;
    arg[1] = 1;
    if (ioctl(g2d_fd, G2D_CMD_MIXER_TASK, (arg)) < 0) {
        divoom_print("G2D_CMD_MIXER_TASK failure!\n");
    }
    else
    {
        memcpy(dest_rgb, dst_ion.virt_addr, out_size);
        ion_flush_cache(info->dst_image_h.fd);
    }

    munmap(dst_ion.virt_addr, out_size);
    munmap(src_ion.virt_addr, in_size);
    ion_memory_release(info->dst_image_h.fd);
    ion_memory_release(info->src_image_h.fd);
    close(ion_fd);
    close(g2d_fd);
    divoom_system_free(info);
    return 1;
    #endif
    
}


int divoom_aw_mpp_stretch_image_direct(uint8 *src_rgb, int src_w, int src_h, int src_type,
        uint8 *dest_rgb, int image_w, int image_h, int dest_w, int dest_h, int dest_type)
{
    int src_fmt = divoom_rk_mpp_convert_color_type(src_type);
    int dst_fmt = divoom_rk_mpp_convert_color_type(dest_type);
    int dest_start_x;
    int dest_start_y;

    if(src_rgb == NULL || dest_rgb == NULL || src_w <= 0 || src_h <= 0 ||
        image_w <= 0 || image_h <= 0 || dest_w <= 0 || dest_h <= 0)
    {
        return 0;
    }
    if(image_w > dest_w || image_h > dest_h)
    {
        return 0;
    }

    if(divoom_rk_mpp_g2d_format_bpp(dst_fmt) == 4)
    {
        divoom_menu_control_init_bgra(dest_rgb, 0xCA, 0xCA, 0xCA, dest_w, dest_h);
    }
    else
    {
        divoom_rk_mpp_zero_buffer(dest_rgb, dst_fmt, dest_w, dest_h);
    }

    dest_start_x = (dest_w - image_w) / 2;
    dest_start_y = (dest_h - image_h) / 2;
    if(divoom_rk_mpp_g2d_blit_scale(src_rgb, src_w, src_h, src_fmt,
            0, 0, src_w, src_h, dest_rgb, dest_w, dest_h, dst_fmt,
            dest_start_x, dest_start_y, image_w, image_h))
    {
        return 1;
    }

    divoom_rk_mpp_cpu_scale_convert(src_rgb, src_w, src_h, src_fmt,
        0, 0, src_w, src_h, dest_rgb, dest_w, dest_h, dst_fmt,
        dest_start_x, dest_start_y, image_w, image_h);
    return 1;
}

/*********************************************************************
  Function:        divoom_rk_mpp_rotate_image
  Description:     对图像进行旋转
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   age_value    //旋转角度
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_rotate_image(uint8 *src_rgb, int *src_w, int *src_h, int src_type,uint8 *dest_rgb, int age_value)
{
    return 1;
}

/*********************************************************************
  Function:        divoom_rk_mpp_mirror_image
  Description:     对图像进行镜像
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_mirror_image(uint8 *src_rgb, int src_w, int src_h, int src_type,uint8 *dest_rgb)
{
    return 0;
}

/*********************************************************************
  Function:        divoom_rk_mpp_crop_image
  Description:     对图像进行裁剪
  Input:           src_rgb
                   src_w
                   src_h
                   src_type
                   dest_rgb
                   start_x    
                   start_y
                   size_x
                   size_y
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_crop_image(uint8 *src_rgb, int src_w, int src_h, int src_type,uint8 *dest_rgb, 
                                int start_x,int start_y,int size_x,int size_y)
{
    const int fmt = divoom_rk_mpp_convert_color_type(src_type);

    if(src_rgb == NULL || dest_rgb == NULL ||
        !divoom_rk_mpp_rect_valid(start_x, start_y, size_x, size_y, src_w, src_h))
    {
        return -1;
    }

    if(divoom_rk_mpp_g2d_blit_scale(src_rgb, src_w, src_h, fmt,
            start_x, start_y, size_x, size_y,
            dest_rgb, size_x, size_y, fmt,
            0, 0, size_x, size_y))
    {
        return 1;
    }

    divoom_rk_mpp_cpu_scale_copy(src_rgb, src_w, src_h, fmt,
        start_x, start_y, size_x, size_y,
        dest_rgb, size_x, size_y, fmt,
        0, 0, size_x, size_y);
    return 1;
}

/*********************************************************************
  Function:        divoom_rk_mpp_crop_image
  Description:     对图像进行裁剪
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   dest_w
                   dest_h
                   
  Return:          0为成功，其它失败原因
**********************************************************************/


void divoom_rk_mpp_crop_image_to_size(uint8 *src_rgb, int src_w, int src_h, uint8 *dest_rgb, int dest_w,int dest_h)
{
    divoom_rk_mpp_crop_image(src_rgb, src_w, src_h, RK_FORMAT_BGRA_8888, dest_rgb, (src_w - dest_w)/2, (src_h - dest_h)/2, dest_w, dest_h);
}


/*********************************************************************
  Function:        divoom_rk_mpp_stretch_crop_image
  Description:     对图像进行缩小,保持图像的比例,如果图像比原来小，就不放大，直接层中显示
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   dest_w
                   dest_h
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_stretch_crop_image(uint8 *src_rgb, int src_w, int src_h, uint8 *dest_rgb, int dest_w, int dest_h)
{
    int src_start_x, src_start_y, src_size_x, src_size_y;
    int dest_start_x, dest_start_y, dest_size_x, dest_size_y;
    
    if(src_w < dest_w || src_h < dest_h)
    {
        //只要图像居中就可以了
        src_start_x = 0;
        src_start_y = 0;
        src_size_x  = src_w;
        src_size_y = src_h;
        dest_start_x = (dest_w - src_w)/2;
        dest_start_y = (dest_h - src_h)/2;
        dest_size_x = dest_w;
        dest_size_y = dest_h;
    }
    else
    {        
        float src_aspect = (float)src_w / src_h;
        float dest_aspect = (float)dest_w / dest_h;

        // 根据宽高比决定缩放策略
        if (src_aspect > dest_aspect) {
            // 宽度主导，按目标宽度计算高度
            src_size_y = src_h;
            src_size_x = dest_w *src_h / dest_h;
            src_size_x = src_size_x & (~3);
        } else {
            // 高度主导，按目标高度计算宽度
            src_size_x = src_w;
            src_size_y = dest_h *src_w / dest_w;
            src_size_y = src_size_y & (~3);
        }
        src_start_x = (src_w - src_size_x)/2;
        src_start_y = (src_h - src_size_y)/2;
        dest_start_x = 0;
        dest_start_y = 0;
        dest_size_x = dest_w;
        dest_size_y = dest_h;
        
    }
    divoom_rk_mpp_crop_stretch_image_direct(G2D_FORMAT_ARGB8888,src_rgb, src_w, src_h, src_start_x, src_start_y, src_size_x, src_size_y, 
                                            dest_rgb, dest_w, dest_h, dest_start_x, dest_start_y, dest_size_x, dest_size_y);
    return 1;
}


/*********************************************************************
  Function:        divoom_rk_mpp_stretch_image
  Description:     对图像进行拉伸，因库限制16倍以下的缩放，增加一层转换
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   dest_w
                   dest_h
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_stretch_image(uint8 *src_rgb, int src_w, int src_h, int src_type,uint8 *dest_rgb, int dest_w, int dest_h)
{
    uint8 *tmp_buf;
    int tmp_width, tmp_high;
    int ret;
    size_t tmp_size;
    int dst_type = RK_FORMAT_RGB_565;
    int dst_fmt = divoom_rk_mpp_convert_color_type(dst_type);
    double w_factor, h_factor;

    if(src_rgb == NULL || dest_rgb == NULL ||
        divoom_rk_mpp_g2d_image_size(divoom_rk_mpp_convert_color_type(src_type), src_w, src_h) == 0 ||
        divoom_rk_mpp_g2d_image_size(dst_fmt, dest_w, dest_h) == 0)
    {
        return 0;
    }

    w_factor = (double)src_w / (double)dest_w;
    h_factor = (double)src_h / (double)dest_h;
    //这个库只支持8倍以下的缩小，所以这里增加判断，如果大于这个值，将分两次缩放
    if(w_factor > 8 || h_factor > 8)
    {
        /* 原先用 dest_*4 当「中间宽高」会与像素语义不符，缩放比仍>8时 G2D 易出横纹/拉丝 */
        if(w_factor > 8)
        {
            tmp_width = (src_w - 1) / 8 + 1;
            if(tmp_width < dest_w)
                tmp_width = dest_w;
        }
        else
        {
            tmp_width = dest_w;
        }
        if(h_factor > 8)
        {
            tmp_high = (src_h - 1) / 8 + 1;
            if(tmp_high < dest_h)
                tmp_high = dest_h;
        }
        else
        {
            tmp_high = dest_h;
        }
        tmp_size = divoom_rk_mpp_g2d_image_size(dst_fmt, tmp_width, tmp_high);
        if(tmp_size == 0)
            return 0;
        tmp_buf = divoom_system_malloc(tmp_size);
        if(tmp_buf == NULL)
            return 0;
        ret = divoom_rk_mpp_stretch_image_direct(src_rgb, src_w, src_h, src_type,
            tmp_buf, tmp_width, tmp_high, dst_type);
        /* tmp→dest 仍可能超过 8 倍，再走本函数拆分 */
        if(ret)
        {
            ret = divoom_rk_mpp_stretch_image(tmp_buf, tmp_width, tmp_high, dst_type,
                dest_rgb, dest_w, dest_h);
        }
        divoom_system_free(tmp_buf);
        return ret;
    }
    else if(w_factor*8 < 1.0 || h_factor*8 < 1.0)
    {
        if(w_factor*8 < 1.0)
        {
            tmp_width = dest_w / sqrt((double)dest_w / src_w);
        }
        else
        {
            tmp_width = dest_w;
        }
        if(h_factor*8 < 1.0)
        {
            tmp_high = dest_h / sqrt((double)dest_h / src_h);
        }
        else
        {
            tmp_high = dest_h;
        }
        if(tmp_width > 1 && (tmp_width & 1))
        {
            tmp_width --;
        }
        if(tmp_high > 1 && (tmp_high & 1))
        {
            tmp_high --;
        }
        tmp_size = divoom_rk_mpp_g2d_image_size(dst_fmt, tmp_width, tmp_high);
        if(tmp_size == 0)
            return 0;
        tmp_buf = divoom_system_malloc(tmp_size);
        if(tmp_buf == NULL)
            return 0;
        ret = divoom_rk_mpp_stretch_image_direct(src_rgb, src_w, src_h, src_type,
            tmp_buf, tmp_width, tmp_high, dst_type);
        if(ret)
        {
            ret = divoom_rk_mpp_stretch_image_direct(tmp_buf, tmp_width, tmp_high,
                dst_type, dest_rgb, dest_w, dest_h, dst_type);
        }
        divoom_system_free(tmp_buf);
        return ret;
    }
    else
    {
        return divoom_rk_mpp_stretch_image_direct(src_rgb, src_w, src_h, src_type,
            dest_rgb, dest_w, dest_h, dst_type);
    }

}

int divoom_rk_mpp_stretch_image_lvfmt(uint8 *src_rgb, int src_w, int src_h,
                                      int src_type, uint8 *dest_rgb,
                                      int dest_w, int dest_h,
                                      lv_color_format_t dest_cf)
{
    int src_fmt = divoom_rk_mpp_convert_color_type(src_type);
    int32 ret;

    if(src_rgb == NULL || dest_rgb == NULL || src_w <= 0 || src_h <= 0 ||
        dest_w <= 0 || dest_h <= 0)
    {
        return 0;
    }

    ret = divoom_rk_mpp_crop_stretch_image_lvfmt(src_fmt, src_rgb,
        src_w, src_h, 0, 0, src_w, src_h, dest_rgb, dest_w, dest_h,
        0, 0, dest_w, dest_h, dest_cf);
    if(ret == 1 &&
        ((src_w == 16 && src_h == 16) ||
        (src_w == 32 && src_h == 32)) &&
        dest_w > src_w && dest_h > src_h &&
        divoom_main_get_show_grid_flag() == 1)
    {
        /* 缩放完成后叠加 20% 网格色，保留原图内容和透明度。 */
        divoom_pixel_format_set_small_image_grid_opa(dest_rgb, (uint32)src_w,
            (uint32)src_h, (uint32)dest_w, (uint32)dest_h, dest_cf, LV_OPA_20);
    }
    return ret;
}

#ifndef DIVOOM_RK_MPP_PREVIEW_DEST_MAX_SIDE
#define DIVOOM_RK_MPP_PREVIEW_DEST_MAX_SIDE 1024
#endif

void divoom_rk_mpp_bgra_scale_nearest_cpu(uint8 *src, int sw, int sh, uint8 *dst, int dw, int dh)
{
	int y, x;

	if (src == NULL || dst == NULL || sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return;
	if (sw == dw && sh == dh)
	{
		memcpy(dst, src, (size_t)sw * (size_t)sh * 4u);
		return;
	}
	for (y = 0; y < dh; y++)
	{
		int sy = (int)(((long)y * (long)sh) / dh);

		if (sy >= sh)
			sy = sh - 1;
		for (x = 0; x < dw; x++)
		{
			int sx = (int)(((long)x * (long)sw) / dw);

			if (sx >= sw)
				sx = sw - 1;
			memcpy(dst + (((long)y * dw) + x) * 4, src + (((long)sy * sw) + sx) * 4, 4);
		}
	}
}

int divoom_rk_mpp_try_preview_bgra_scale_cpu(uint8 *src, int sw, int sh, uint8 *dst, int dw, int dh)
{
	if (src == NULL || dst == NULL || sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return 0;
	if (dw > DIVOOM_RK_MPP_PREVIEW_DEST_MAX_SIDE || dh > DIVOOM_RK_MPP_PREVIEW_DEST_MAX_SIDE)
		return 0;
	divoom_rk_mpp_bgra_scale_nearest_cpu(src, sw, sh, dst, dw, dh);
	return 1;
}

/*********************************************************************
  Function:        divoom_rk_mpp_get_disp_image_info
  Description:     对图像进行拉伸， 要保持原始图像的比例
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   dest_w
                   dest_h
                   fill_flag    0:以dest_w为准， 1：以dest_h
  Return:          0为成功，其它失败原因
**********************************************************************/


#ifndef DIVOOM_SIMULATOR
int divoom_rk_mpp_get_disp_image_info(uint8 *src_rgb, int src_w, int src_h, int src_type,uint8 *dest_rgb, int *dest_w, int *dest_h, int dest_type, int fill_flag)
{
    uint8 *tmp_buf;
    int image_w, image_h,  tmp_width, tmp_high;
    double w_factor, h_factor;

    if(src_w *(*dest_h) > src_h *(*dest_w))
    {
        //以宽度为基准
        image_w = *dest_w;
        image_h = (*dest_w) * src_h/src_w;
        
    }
    else
    {
        //以高度为基准
        image_h = *dest_h;
        image_w = (*dest_h) * src_w/src_h;
    }
    divoom_print("info: %d, %d, %d, %d, %d, %d\n", src_w, src_h, *dest_w, *dest_h, image_w, image_h);
    
    w_factor = (double)src_w / (double)image_w;
    h_factor = (double)src_h / (double)image_h;
    //这个库只支持8倍以下的缩小，所以这里增加判断，如果大于这个值，将分两次缩放
    if(w_factor > 8 || h_factor > 8)
    {
       if(w_factor > 8)
       {
           tmp_width = (src_w + 7) / 8;
           if(tmp_width < image_w)
               tmp_width = image_w;
       }
       else
       {
           tmp_width = image_w;
       }
       if(h_factor > 8)
       {
           tmp_high = (src_h + 7) / 8;
           if(tmp_high < image_h)
               tmp_high = image_h;
       }
       else
       {
           tmp_high = image_h;
       }

       tmp_buf = divoom_system_malloc((size_t)tmp_width * (size_t)tmp_high * 4u);
       if(tmp_buf == NULL)
           return 0;
       divoom_aw_mpp_stretch_image_direct(src_rgb, src_w, src_h, src_type, tmp_buf, tmp_width, tmp_high, tmp_width, tmp_high, src_type);
       divoom_aw_mpp_stretch_image_direct(tmp_buf, tmp_width, tmp_high, src_type, dest_rgb, image_w, image_h, *dest_w, *dest_h, src_type);
       divoom_system_free(tmp_buf);
    }
    else if(w_factor*8 < 1.0 || h_factor*8 < 1.0)
    {
       if(w_factor*8 < 1.0)
       {
           tmp_width = image_w / sqrt(image_w/src_w);
       }
       else
       {
           tmp_width = image_w;
       }
       if(h_factor*8 < 1.0)
       {
           tmp_high = image_h / sqrt(image_h/src_h);
       }
       else
       {
           tmp_high = image_h;
       }
       if(tmp_width &1)
       {
           tmp_width --;
       }
       if(tmp_high &1)
       {
           tmp_high --;
       }
       
       tmp_buf = divoom_system_malloc(tmp_width *tmp_high * 4);
       divoom_aw_mpp_stretch_image_direct(src_rgb, src_w, src_h, src_type, tmp_buf, tmp_width, tmp_high, tmp_width, tmp_high, src_type);
       divoom_aw_mpp_stretch_image_direct(tmp_buf, tmp_width, tmp_high, src_type, dest_rgb, image_w, image_h, *dest_w, *dest_h, src_type);
       divoom_system_free(tmp_buf);
    }
    else
    {
       
       divoom_aw_mpp_stretch_image_direct(src_rgb, src_w, src_h, src_type, dest_rgb, image_w, image_h, *dest_w, *dest_h, src_type);
    }
    
}
#endif /* !DIVOOM_SIMULATOR */


/*********************************************************************
  Function:        divoom_rk_mpp_get_stretch_image_width_high
  Description:     计算保持原始图像比例的目标尺寸
  Input:           src_w  - 源图像宽度
                   src_h  - 源图像高度
                   dest_w - 目标最大宽度
                   dest_h - 目标最大高度
  Output:          mid_w  - 计算后的实际宽度
                   mid_h  - 计算后的实际高度
  Return:          0 - 不需要缩放(比例相同)
                   1 - 需要缩放
**********************************************************************/
int divoom_rk_mpp_get_stretch_image_width_high(int src_w, int src_h, int dest_w, int dest_h, int* mid_w, int* mid_h)
{
    // 检查源图像和目标图像是否已经是相同比例
    if (src_w * dest_h == src_h * dest_w) {
        *mid_w = src_w;
        *mid_h = src_h;
        return 0;
    }

    // 计算宽高比
    float src_ratio = (float)src_w / src_h;
    float dest_ratio = (float)dest_w / dest_h;

    // 根据目标区域的约束确定输出尺寸
    if(src_w > dest_w || src_h > dest_h)
    {
        if (src_ratio < dest_ratio) {
            // 以高度为基准进行缩放
            *mid_h = dest_h;
            *mid_w = (int)(src_w * dest_h / src_h);
        } else {
            // 以宽度为基准进行缩放
            *mid_w = dest_w;
            *mid_h = (int)(src_h *dest_w / src_w);
        }
    }
    else
    {
        if (src_ratio > dest_ratio) {
            // 以宽度为基准进行缩放
            *mid_w = dest_w;
            *mid_h = (int)(dest_w *src_h / src_w);
        } else {
            // 以高度为基准进行缩放
            *mid_h = dest_h;
            *mid_w = (int)(dest_h * src_w / src_h);
        }
    }

    // 确保宽度是4的倍数(为了对齐)
    *mid_w = (*mid_w & (~3));
    
    return 1;
}


/*********************************************************************
  Function:        divoom_rk_mpp_stretch_image_data
  Description:     对原始图像进行拉伸，按特定的比例进行输出，如果比例不合，将会拉伸一下
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   dest_w
                   dest_h
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_stretch_raw_picture_data(uint8 *src_rgb, int src_w, int src_h,uint8 *dest_rgb, int dest_w, int dest_h)
{
    return divoom_rk_mpp_stretch_image(src_rgb, src_w, src_h,
        RK_FORMAT_BGRA_8888, dest_rgb, dest_w, dest_h);
}

/*********************************************************************
  Function:        divoom_rk_mpp_save_jpeg_file
  Description:     将图像压缩成相关大小再保存到文件中
  Input:           file_name        //要保存的文件名
                   src_rgb          //原始图像的BGRA BUF
                   src_w            //
                   src_h
                   dest_w
                   dest_h
  Return:          0为失败，其它成功
**********************************************************************/


int divoom_rk_mpp_save_jpeg_file(char *file_name, uint8 *src_rgb, int src_w, int src_h, int dest_w, int dest_h)
{
    uint8 *dest_rgb;

    if(src_rgb == NULL)
    {
        divoom_print("aj test! ERR\n");
    }
    if(src_w == dest_w && src_h == dest_h)
    {
        divoom_encode_rgb_to_jpeg(file_name, src_rgb, dest_w, dest_h);
        return 0;
    }
    
    dest_rgb = divoom_system_malloc(dest_w * dest_h *3);
    if(dest_rgb == NULL)
    {
        return 0;
    }

    divoom_rk_mpp_stretch_image_direct(src_rgb, src_w, src_h,
        RK_FORMAT_BGRA_8888, dest_rgb, dest_w, dest_h, RK_FORMAT_RGB_888);
    //下面这里压缩成JPEG，再保存到文件中,暂时使用webp
    divoom_encode_rgb_to_jpeg(file_name, dest_rgb, dest_w, dest_h);
    divoom_system_free(dest_rgb);
}
 

/*********************************************************************
  Function:        divoom_encode_rgb_to_jpeg
  Description:     将RGB数据保存在JPG文件中，rgb_buffer必须是RGB格式
  Input:           file_name        //要保存的文件名
                   rgb_buffer          //原始图像的RGB BUF
                   width            //
                   height
  Return:          none
**********************************************************************/


void divoom_encode_rgb_to_jpeg(char *file_name, unsigned char *rgb_buffer, int width, int height) 
{
    unsigned char *jpeg_buffer = NULL;
    unsigned long jpeg_size;
    tjhandle tj_instance = tjInitCompress();
    if (!tj_instance) {
        divoom_print( "Error initializing TurboJPEG.\n");
        return ;
    }
    
    if (tjCompress2(tj_instance, rgb_buffer, width, 0, height, TJPF_BGRA, &jpeg_buffer, &jpeg_size, TJSAMP_420, 90, TJFLAG_FASTUPSAMPLE) != 0) {
        divoom_print("Error compressing image: %s\n", tjGetErrorStr());
        tjDestroy(tj_instance);
        return ;
    }
    tjDestroy(tj_instance);
    if(jpeg_buffer == NULL)
    {
        divoom_print( "BUFFER IS null.\n");
        return;
    }
    divoom_app_com_write_file(file_name, jpeg_buffer, jpeg_size);
    tjFree(jpeg_buffer);
}

 
uint8 *divoom_rk_mpp_get_png_info(const char *filename, int *width, int *height) 
{
#ifndef DIVOOM_SIMULATOR
    return divoom_aw_decode_png_file(filename, width, height);
#else
    int file_len = 0;
    uint8 *file_buf = NULL;
    unsigned char *img_data = NULL;
    uint8 *out_buf = NULL;
    unsigned w = 0;
    unsigned h = 0;
    unsigned err;
    size_t out_size;

    if (filename == NULL || width == NULL || height == NULL)
    {
        return NULL;
    }

    file_buf = (uint8 *)divoom_app_com_read_file((char *)filename, &file_len);
    if (file_buf == NULL || file_len < 8)
    {
        if (file_buf != NULL)
        {
            divoom_system_free(file_buf);
        }
        return NULL;
    }

    err = lodepng_decode32(&img_data, &w, &h, (const unsigned char *)file_buf, (size_t)file_len);
    divoom_system_free(file_buf);
    file_buf = NULL;

    if (err != 0)
    {
        divoom_print("lodepng err %u: %s (%s)\n", err, lodepng_error_text(err), filename);
        if (img_data != NULL)
        {
            lv_mem_free(img_data);
        }
        return NULL;
    }

    if (w == 0 || h == 0 || w > 8192 || h > 8192)
    {
        if (img_data != NULL)
        {
            lv_mem_free(img_data);
        }
        return NULL;
    }

    out_size = (size_t)w * (size_t)h * 4u;
    out_buf = (uint8 *)divoom_system_malloc((uint32)out_size);
    if (out_buf == NULL)
    {
        if (img_data != NULL)
        {
            lv_mem_free(img_data);
        }
        return NULL;
    }
    /*
     * 仿真 lodepng 默认用 LVGL/libc 分配器，真机 pngdecode 返回 divoom_system_malloc。
     * 这里统一复制到系统堆，Native Lua 图片缓存退出时即可用同一释放路径。
     */
    memcpy(out_buf, img_data, out_size);
    lv_mem_free(img_data);
    *width = (int)w;
    *height = (int)h;
    return out_buf;
#endif
}

uint8 *divoom_rk_mpp_get_png_lvfmt_to_buf_info(const uint8 *buf, int buf_len,
    int *width, int *height, lv_color_format_t cf, uint8 *out_buf,
    uint32 out_size)
{
#ifndef DIVOOM_SIMULATOR
    return divoom_aw_decode_png_buf_to_lvfmt((const char *)buf, buf_len, width,
        height, out_buf, out_size, cf);
#else
    unsigned char *img_data = NULL;
    unsigned w = 0;
    unsigned h = 0;
    unsigned err;
    uint32 px_count;

    if (buf == NULL || buf_len <= 0 || width == NULL || height == NULL ||
        out_buf == NULL || (cf != LV_COLOR_FORMAT_RGB565 &&
        cf != LV_COLOR_FORMAT_RGB565A8))
    {
        return NULL;
    }

    err = lodepng_decode32(&img_data, &w, &h, (const unsigned char *)buf,
        (size_t)buf_len);
    if (err != 0)
    {
        divoom_print("lodepng err %u: %s\n", err, lodepng_error_text(err));
        if (img_data != NULL)
        {
            lv_mem_free(img_data);
        }
        return NULL;
    }
    if (w == 0 || h == 0 || w > 8192 || h > 8192)
    {
        lv_mem_free(img_data);
        return NULL;
    }
    if (divoom_pixel_format_size((uint32)w, (uint32)h, cf) == 0 ||
        divoom_pixel_format_size((uint32)w, (uint32)h, cf) > out_size)
    {
        lv_mem_free(img_data);
        return NULL;
    }

    px_count = (uint32)w * (uint32)h;
    /* lodepng 返回缓冲由 LVGL 分配，解完立即释放；out_buf 生命周期归调用方。 */
    if (cf == LV_COLOR_FORMAT_RGB565A8)
    {
        divoom_rgba8888_to_rgb565a8_buf(img_data, out_buf, px_count);
    }
    else
    {
        uint32 i;

        for (i = 0; i < px_count; i++)
        {
            uint16 c = divoom_rgb888_to_rgb565(img_data[i * 4U],
                img_data[i * 4U + 1U], img_data[i * 4U + 2U]);

            divoom_store_rgb565_le(out_buf + i * DIVOOM_APP_RGB565_BYTES_PER_PX,
                c);
        }
    }
    lv_mem_free(img_data);
    *width = (int)w;
    *height = (int)h;
    return out_buf;
#endif
}

uint8 *divoom_rk_mpp_get_png_lvfmt_info(const char *filename, int *width,
    int *height, lv_color_format_t cf)
{
#ifndef DIVOOM_SIMULATOR
    divoom_aw_png_decode_buf_t reuse;
    uint8 *out_buf;

    if(filename == NULL || width == NULL || height == NULL ||
        (cf != LV_COLOR_FORMAT_RGB565 && cf != LV_COLOR_FORMAT_RGB565A8))
    {
        return NULL;
    }

    memset(&reuse, 0, sizeof(reuse));
    out_buf = divoom_aw_decode_png_file_to_lvfmt_reuse(filename, width, height,
        &reuse, cf);
    if(out_buf == NULL)
    {
        divoom_aw_png_decode_buf_release(&reuse);
        return NULL;
    }
    reuse.out_buf_owner = 0;
    divoom_aw_png_decode_buf_release(&reuse);
    return out_buf;
#else
    uint8 *rgba_buf;
    uint8 *out_buf;
    uint32 px_count;
    int w = 0;
    int h = 0;

    if(filename == NULL || width == NULL || height == NULL ||
        (cf != LV_COLOR_FORMAT_RGB565 && cf != LV_COLOR_FORMAT_RGB565A8))
    {
        return NULL;
    }

    rgba_buf = divoom_rk_mpp_get_png_info(filename, &w, &h);
    if(rgba_buf == NULL || w <= 0 || h <= 0)
    {
        return NULL;
    }

    px_count = (uint32)w * (uint32)h;
    out_buf = divoom_system_malloc(divoom_pixel_format_size((uint32)w,
        (uint32)h, cf));
    if(out_buf == NULL)
    {
        divoom_system_free(rgba_buf);
        return NULL;
    }

    /*
     * PNG 统一从 RK MPP 层转成 LVGL 侧 565 格式，保留 alpha 到 RGB565A8
     * 平面，避免上层再按 RGBA 自己转换。
     */
    if(cf == LV_COLOR_FORMAT_RGB565A8)
    {
        divoom_rgba8888_to_rgb565a8_buf(rgba_buf, out_buf, px_count);
    }
    else
    {
        uint32 i;

        for(i = 0; i < px_count; i++)
        {
            uint16 c = divoom_rgb888_to_rgb565(rgba_buf[i * 4U],
                rgba_buf[i * 4U + 1U], rgba_buf[i * 4U + 2U]);
            divoom_store_rgb565_le(out_buf + i * DIVOOM_APP_RGB565_BYTES_PER_PX,
                c);
        }
    }

    divoom_system_free(rgba_buf);
    *width = w;
    *height = h;
    return out_buf;
#endif
}
 
/*********************************************************************
  Function:        divoom_rk_mpp_decode_jpeg_rgb565_internal
  Description:     从JPEG的图像数据，输出RGB565缓冲区
  Input:           buf        //要保存的文件名
                   buf_len            //
                   image_w
                   image_h
                   rgb_type
                   image_data
  Return:          返回RGB565缓冲区
**********************************************************************/


static uint8 *divoom_rk_mpp_decode_jpeg_rgb565_internal(char *buf, int buf_len,
    int *image_w, int *image_h, uint8 *image_data)
{
#ifndef DIVOOM_SIMULATOR
    {
        uint8 *rgb565_buf;
        int width = 0;
        int height = 0;

        if(image_data != NULL)
        {
            uint32 cap;

            if(!divoom_aw_parse_jpeg_sof_info(buf, buf_len, &width, &height))
            {
                return NULL;
            }
            cap = (uint32)width * (uint32)height * DIVOOM_APP_RGB565_BYTES_PER_PX;
            return divoom_aw_decode_jpeg_rgb565_to_buf(buf, buf_len, image_w, image_h, image_data, cap);
        }
        rgb565_buf = divoom_aw_decode_jpeg_rgb565_buf(buf, buf_len, &width, &height);
        if(rgb565_buf == NULL)
        {
            return NULL;
        }
        if(image_w)
        {
            *image_w = width;
        }
        if(image_h)
        {
            *image_h = height;
        }
        return rgb565_buf;
    }
#else
    int subsamp;
    int width;
    int height;
    tjhandle tj_instance;
    uint8 *rgb_buf;
    uint32 px_count;
    int allocated_output = 0;

    tj_instance = tjInitDecompress();
    if (!tj_instance)
    {
        divoom_print("Error initializing TurboJPEG.\n");
        return NULL;
    }

    if (tjDecompressHeader2(tj_instance, buf, buf_len, &width, &height, &subsamp) != 0)
    {
        tjDestroy(tj_instance);
        return NULL;
    }

    px_count = (uint32)width * (uint32)height;
    rgb_buf = (uint8 *)malloc((size_t)px_count * 3u);
    if (!rgb_buf)
    {
        divoom_print("Error allocating RGB temp image.\n");
        tjDestroy(tj_instance);
        return NULL;
    }
    if (image_data == NULL)
    {
        image_data = (unsigned char *)divoom_system_malloc((size_t)px_count * 2u);
        allocated_output = 1;
    }
    if (!image_data)
    {
        divoom_print("Error allocating memory for image.\n");
        tjDestroy(tj_instance);
        free(rgb_buf);
        return NULL;
    }

    if (tjDecompress2(tj_instance, buf, buf_len, rgb_buf, width, 0, height, TJPF_RGB,
            TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE) != 0)
    {
        divoom_print("Error decompressing JPEG image: %s\n", tjGetErrorStr());
        tjDestroy(tj_instance);
        free(rgb_buf);
        if (allocated_output)
        {
            divoom_system_free(image_data);
        }
        return NULL;
    }

    divoom_rgb888_to_rgb565_buf(rgb_buf, image_data, px_count);
    free(rgb_buf);
    tjDestroy(tj_instance);
    if (image_w)
    {
        *image_w = width;
    }
    if (image_h)
    {
        *image_h = height;
    }
    return image_data;
#endif
}

uint8 *divoom_rk_mpp_decode_jpeg_rgb565_info(char *buf, int buf_len, int *image_w, int *image_h)
{
#ifndef DIVOOM_SIMULATOR
    return divoom_aw_decode_jpeg_rgb565_buf(buf, buf_len, image_w, image_h);
#else
    return divoom_rk_mpp_decode_jpeg_rgb565_internal(buf, buf_len, image_w,
        image_h, NULL);
#endif
}

uint8 *divoom_rk_mpp_decode_jpeg_rgb565_to_buf_info(char *buf, int buf_len,
    int *image_w, int *image_h, uint8 *out_buf, uint32 out_size)
{
#ifndef DIVOOM_SIMULATOR
    return divoom_aw_decode_jpeg_rgb565_to_buf(buf, buf_len, image_w, image_h,
        out_buf, out_size);
#else
    tjhandle tj_instance;
    int subsamp;
    int width;
    int height;
    uint32 need_size;
    char *decode_buf;
    char *input_copy;
    uint8 *ret_buf;

    if (buf == NULL || buf_len <= 0 || out_buf == NULL || out_size == 0 ||
        image_w == NULL || image_h == NULL)
    {
        return NULL;
    }

    tj_instance = tjInitDecompress();
    if (tj_instance == NULL)
    {
        return NULL;
    }
    width = 0;
    height = 0;
    if (tjDecompressHeader2(tj_instance, buf, buf_len, &width, &height,
            &subsamp) != 0)
    {
        tjDestroy(tj_instance);
        return NULL;
    }
    tjDestroy(tj_instance);
    need_size = (uint32)width * (uint32)height * DIVOOM_APP_RGB565_BYTES_PER_PX;
    if (need_size == 0 || need_size > out_size)
    {
        return NULL;
    }

    decode_buf = buf;
    input_copy = NULL;
    if ((uint8 *)buf == out_buf)
    {
        /* 仿真 TurboJPEG 不保证输入/输出同址安全；内部保护压缩输入，外部仍复用 base pool。 */
        input_copy = (char *)malloc((size_t)buf_len);
        if (input_copy == NULL)
        {
            return NULL;
        }
        memcpy(input_copy, buf, (size_t)buf_len);
        decode_buf = input_copy;
    }
    ret_buf = divoom_rk_mpp_decode_jpeg_rgb565_internal(decode_buf, buf_len,
        image_w, image_h, out_buf);
    if (input_copy != NULL)
    {
        free(input_copy);
    }
    return ret_buf;
#endif
}

uint8 *divoom_rk_mpp_decode_jpeg_rgb565_to_buf(char *buf, int buf_len,
    int *image_w, int *image_h, uint8 *out_buf, uint32 out_size)
{
    /* 兼容旧调用名，实际仍走统一的 JPEG RGB565 到指定缓冲区解码封装。 */
    return divoom_rk_mpp_decode_jpeg_rgb565_to_buf_info(buf, buf_len, image_w,
        image_h, out_buf, out_size);
}

#pragma pack(push, 1)
typedef struct {
    uint16_t signature;      // 'BM'
    uint32_t fileSize;       // 文件大小
    uint32_t reserved;       // 保留字段
    uint32_t dataOffset;     // 数据偏移量
} BmpFileHeader;

typedef struct {
    uint32_t size;          // 信息头大小
    int32_t width;          // 图像宽度
    int32_t height;         // 图像高度
    uint16_t planes;        // 颜色平面数
    uint16_t bitsPerPixel;  // 每像素位数
    uint32_t compression;   // 压缩方式
    uint32_t imageSize;     // 图像数据大小
    int32_t xPixelsPerMeter;
    int32_t yPixelsPerMeter;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
} BmpInfoHeader;
#pragma pack(pop)

/*********************************************************************
  Function:        divoom_decode_bmp_file
  Description:     从文件中读取BMP文件的图像数据，输出bgra缓冲区
  Input:           file_name        //要保存的文件名
                   image_w            //
                   image_h
                   image_data
  Return:          返回bgra缓冲区
**********************************************************************/

uint8* divoom_decode_bmp_file(const char* filename, int *image_w, int *image_h, uint8 *image_data) {
    int malloc_flag = 0;

    FILE* file = divoom_port_file_open(filename, "rb");
    if (!file) {
        divoom_print("it fails to open : %s\n", filename);
        return NULL;
    }

    BmpFileHeader fileHeader;
    BmpInfoHeader infoHeader;

    // 读取文件头
    if (divoom_port_file_read(&fileHeader, sizeof(fileHeader), 1, file) != 1) {
        divoom_print("it fails to read : %s\n", filename);
        divoom_port_file_close(file);
        return NULL;
    }

    // 检查BMP签名
    if (fileHeader.signature != 0x4D42) { // "BM"
        divoom_print("it fails to read : %s\n", filename);
        divoom_port_file_close(file);
        return NULL;
    }

    // 读取信息头
    if (divoom_port_file_read(&infoHeader, sizeof(infoHeader), 1, file) != 1) {
        divoom_print("it fails to read : %s\n", filename);
        divoom_port_file_close(file);
        return NULL;
    }

    // 检查是否为32位图像
    if (infoHeader.bitsPerPixel != 32) {
        divoom_print("it fails to read 32 bmp: %s\n", filename);
        divoom_port_file_close(file);
        return NULL;
    }


    *image_w = infoHeader.width;
    *image_h = abs(infoHeader.height);
    
    int dataSize = (infoHeader.width) * (*image_h) * 4;
    if(image_data == NULL)
    {
        malloc_flag = 1;
        image_data = (uint8_t*)divoom_system_malloc(dataSize);
    }
    if (image_data == NULL) {
        divoom_print("it fails to malloc 32 bmp: %s\n", filename);
        divoom_port_file_close(file);
        return NULL;
    }

    // 移动到图像数据开始位置
    divoom_port_file_seek(file, fileHeader.dataOffset, SEEK_SET);

    // 读取图像数据
    for (uint32_t y = 0; y < *image_h; y++) {
        // 计算当前行的位置
        uint32_t destY = (infoHeader.height < 0) ? y : (*image_h - 1 - y);
        uint8_t* rowPtr = image_data + (destY * infoHeader.width * 4);
        
        // 直接读取整行数据
        if (divoom_port_file_read(rowPtr, 4, infoHeader.width, file) != infoHeader.width) {
            divoom_print("it fails to read 32 bmp: %s\n", filename);
            if(malloc_flag)
            {
                divoom_system_free(image_data);
            }
            divoom_port_file_close(file);
            return NULL;
        }
    }

    divoom_port_file_close(file);
    return image_data;
}

uint8* divoom_decode_bmp_file_lvfmt(const char* filename, int *image_w,
                                    int *image_h, uint8 *image_data,
                                    lv_color_format_t cf)
{
    uint8 *tmp_bgra;
    int w = 0;
    int h = 0;

    if(image_w == NULL || image_h == NULL || image_data == NULL)
    {
        return NULL;
    }
    if(cf != LV_COLOR_FORMAT_RGB565 && cf != LV_COLOR_FORMAT_RGB565A8)
    {
        return NULL;
    }

    tmp_bgra = divoom_decode_bmp_file(filename, &w, &h, NULL);
    if(tmp_bgra == NULL)
    {
        return NULL;
    }
    divoom_rk_mpp_stretch_image_lvfmt(tmp_bgra, w, h, RK_FORMAT_BGRA_8888,
        image_data, *image_w, *image_h, cf);
    *image_w = w;
    *image_h = h;
    divoom_system_free(tmp_bgra);
    return image_data;
}



/*********************************************************************
  Function:        divoom_rk_mpp_get_jpeg_info
  Description:     从文件中读取JPEG文件的图像数据，输出RGB565缓冲区
  Input:           file_name        //要保存的文件名
                   image_w            //
                   image_h
  Return:          返回RGB565缓冲区
**********************************************************************/


uint8 *divoom_rk_mpp_decode_jpeg_file(char *file_name, int *image_w, int *image_h)
{
    int file_len;
    uint8 *read_buffer = divoom_app_com_read_file(file_name, &file_len);
    if(read_buffer == NULL)
    {
        divoom_print("it fails to read file: %s\n", file_name);
        return NULL;
    }
    unsigned char *image_data = divoom_rk_mpp_decode_jpeg_rgb565_info(
        (char *)read_buffer, file_len, image_w, image_h);
    divoom_system_free(read_buffer);
    return image_data;
}

/*********************************************************************
  Function:        divoom_rk_mpp_get_jpeg_info
  Description:     从文件中读取图片数据；JPG/JPEG 输出RGB565，其它格式保持原解码格式
  Input:           file_name        //要保存的文件名
                   image_w            //
                   image_h
  Return:          JPG/JPEG 返回RGB565，其它格式返回原有BGRA/RGBA类缓冲区
**********************************************************************/


uint8 *divoom_rk_mpp_get_jpeg_info(char *file_name, int *image_w, int *image_h)
{
    char file_ext_name[16];
    uint8 *out_buf;
    int frame_time;

    divoom_app_com_get_file_ex_name(file_name, file_ext_name);
    if(strcasecmp(file_ext_name,"bmp") == 0) 
    {
        return divoom_decode_bmp_file(file_name, image_w, image_h, NULL);
    }
    if(strcasecmp(file_ext_name,"mp4") == 0) 
    {
        out_buf = divoom_system_malloc(DIVOOM_SCREEN_WIDTH * DIVOOM_SCREEN_HIGH * 4);
        *image_w = DIVOOM_SCREEN_WIDTH;
        *image_h = DIVOOM_SCREEN_HIGH;
        divoom_decode_movie_review_pic_get_buf(file_name, out_buf, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH, 0, 0);
        return out_buf;
    }
    if(strcasecmp(file_ext_name,"jpg") == 0 || strcasecmp(file_ext_name,"jpeg") == 0)
    {
        return divoom_rk_mpp_decode_jpeg_file(file_name, image_w, image_h);
    }
    else if(strcasecmp(file_ext_name,"webp") == 0)
    {
        void *image_handle = divoom_libwebp_decode_init(file_name);
        if(image_handle  == NULL)
        {
            divoom_print("it fails to decode: %s\n", file_name);
            return NULL;
        }
        divoom_libwebp_decode_get_frame_info(image_handle, image_w, image_h);
        out_buf = divoom_libwebp_decode_get_frame_buffer(image_handle, &frame_time, NULL, NULL);
        divoom_libwebp_decode_exit(image_handle);    
        return out_buf;
    }
    else if(strcasecmp(file_ext_name,"HEIF") == 0)
    {
    }
    else if(strcasecmp(file_ext_name,"gif") == 0)
    {
        void *gif_handle = divoom_gif_lib_init(file_name, NULL, 0);

        if(gif_handle)
        {
            uint8 *rgb565a8_buf, *bgra_buf;
            int pixel_count, i;

            divoom_gif_lib_get_pic_info(gif_handle, image_w, image_h, NULL);
            if(*image_w <= 0 || *image_h <= 0)
            {
                divoom_gif_lib_exit(gif_handle);
                return NULL;
            }
            pixel_count = (*image_w) * (*image_h);
            rgb565a8_buf = divoom_system_malloc(pixel_count * 3);
            if(rgb565a8_buf == NULL)
            {
                divoom_gif_lib_exit(gif_handle);
                return NULL;
            }
            if(divoom_gif_lib_get_pic_data(gif_handle, rgb565a8_buf, NULL, NULL, 1, 1) != 1)
            {
                divoom_system_free(rgb565a8_buf);
                divoom_gif_lib_exit(gif_handle);
                return NULL;
            }
            divoom_gif_lib_exit(gif_handle);
            bgra_buf = divoom_system_malloc(pixel_count * 4);
            if(bgra_buf == NULL)
            {
                divoom_system_free(rgb565a8_buf);
                return NULL;
            }
            /* GIF 输出为 RGB565 + A8 平面，缩略图缩放/编码需要 BGRA8888。 */
            for(i = 0; i < pixel_count; i++)
            {
                uint16 color = (uint16)rgb565a8_buf[i * 2] |
                    ((uint16)rgb565a8_buf[i * 2 + 1] << 8);
                uint8 r = (color >> 11) & 0x1f;
                uint8 g = (color >> 5) & 0x3f;
                uint8 b = color & 0x1f;

                bgra_buf[i * 4] = (b << 3) | (b >> 2);
                bgra_buf[i * 4 + 1] = (g << 2) | (g >> 4);
                bgra_buf[i * 4 + 2] = (r << 3) | (r >> 2);
                bgra_buf[i * 4 + 3] = rgb565a8_buf[pixel_count * 2 + i];
            }
            divoom_system_free(rgb565a8_buf);
            return bgra_buf;
        }
        return NULL;
    }
    else if(strcasecmp(file_ext_name,"png") == 0)
    {
        uint8 *rgb_buf = divoom_rk_mpp_get_png_info(file_name, image_w, image_h);
        return rgb_buf;
            
    }
    else
    {
        //这个位置认为是视频，将解码第一帧
        return NULL;
    }
}

/*********************************************************************
  Function:        divoom_rk_mpp_get_pic_width_info
  Description:     从文件中读取JPEG文件的图像数据，得到宽高信息
  Input:           file_name        //要保存的文件名
                   image_w            //
                   image_h
  Return:          返回RGBA缓冲区
**********************************************************************/


void divoom_rk_mpp_get_pic_width_info(char *file_name, int *image_w, int *image_h)
{
    char file_ext_name[16];
    uint8 *out_buf;
    int frame_time;

    divoom_app_com_get_file_ex_name(file_name, file_ext_name);
    
    if(strcasecmp(file_ext_name,"jpg") == 0 || strcasecmp(file_ext_name,"jpeg") == 0)
    {
        int file_len;
        uint8 *read_buffer = divoom_app_com_read_file(file_name, &file_len);
        if(read_buffer == NULL)
        {
            divoom_print("it fails to read file: %s\n", file_name);
            return ;
        }
        unsigned char *image_data = divoom_rk_mpp_decode_jpeg_rgb565_info(
            (char *)read_buffer, file_len, image_w, image_h);
        divoom_system_free(read_buffer);
        divoom_system_free(image_data);
        return ;
    }
    else if(strcasecmp(file_ext_name,"webp") == 0)
    {
        void *image_handle = divoom_libwebp_decode_init(file_name);
        if(image_handle  == NULL)
        {
            divoom_print("it fails to decode: %s\n", file_name);
            return ;
        }
        divoom_libwebp_decode_get_frame_info(image_handle, image_w, image_h);
        divoom_libwebp_decode_exit(image_handle);    
        return ;
    }
    else if(strcasecmp(file_ext_name,"HEIF") == 0)
    {
    }
    else if(strcasecmp(file_ext_name,"gif") == 0)
    {
        void *gif_handle = divoom_gif_lib_init(file_name, NULL, 0);

        if(gif_handle)
        {
            divoom_gif_lib_get_pic_info(gif_handle, image_w, image_h, NULL);
            divoom_gif_lib_exit(gif_handle);
            return ;
        }
    }
    else if(strcasecmp(file_ext_name,"png") == 0)
    {
        uint8 *rgb_buf = divoom_rk_mpp_get_png_info(file_name, image_w, image_h);
        divoom_system_free(rgb_buf);
        return ;
            
    }
    else
    {
        //这个位置认为是视频，将解码第一帧
        return ;
    }
}
