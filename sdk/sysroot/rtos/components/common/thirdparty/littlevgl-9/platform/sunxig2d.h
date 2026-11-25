/**
 * @file sunxig2d.h
 *
 */

#ifndef SUNXIG2D_H
#define SUNXIG2D_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_SUNXIFB_G2D
/*********************
 *      INCLUDES
 *********************/

#include "lvgl.h"
#include "lvgl_private.h"

#ifdef OS_LINUX
#ifdef LVGL_USE_BSP_DRIVER
#include <sunxi-g2d.h>
#elif defined CONF_G2D_VERSION_OLD
#include "g2d_driver.h"
#else
#include "g2d_driver_enh.h"
#endif
#else
#include "g2d_driver.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
    SUNXI_G2D_LIMIT_FILL,
    SUNXI_G2D_LIMIT_OPA_FILL,
    SUNXI_G2D_LIMIT_BLIT,
    SUNXI_G2D_LIMIT_BLEND,
    SUNXI_G2D_LIMIT_SCALE
} sunxi_g2d_limit;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
int32_t sunxifb_g2d_init(uint32_t bits_per_pixel);

void sunxifb_g2d_deinit(void);

int32_t sunxifb_g2d_get_limit(sunxi_g2d_limit limit);

int32_t sunxifb_g2d_blit_to_fb(uintptr_t src_buf, uint32_t src_w, uint32_t src_h,
        uint32_t src_cx, uint32_t src_cy, uint32_t src_cw, uint32_t src_ch,
        uintptr_t dst_buf, uint32_t dst_w, uint32_t dst_h, uint32_t dst_cx,
        uint32_t dst_cy, uint32_t dst_cw, uint32_t dst_ch,
        g2d_blt_flags_h rotated);

int32_t sunxifb_g2d_transform(uint8_t *dst_buf, int32_t dst_w, int32_t dst_h,
        lv_color_format_t dst_cf, uint8_t *src_buf_y, uint8_t *src_buf_u,
        uint8_t *src_buf_v, int32_t src_w, int32_t src_h, uint32_t src_cx,
        uint32_t src_cy, uint32_t src_cw, uint32_t src_ch,
        lv_color_format_t src_cf);

#ifdef USE_SUNXI_G2D_DRAW
int32_t sunxifb_g2d_fill(uint8_t *dst_buf, const lv_area_t *dst_area, lv_color_format_t dst_cf,
        const lv_area_t *draw_area, lv_color_t draw_color, lv_opa_t opa);

int32_t sunxifb_g2d_blit(uint8_t *dst_buf, const lv_area_t *dst_area, lv_color_format_t dst_cf,
        uint8_t *src_buf, const lv_area_t *src_area, lv_color_format_t src_cf,
        const lv_area_t *draw_area, lv_opa_t opa);

int32_t sunxifb_g2d_scale(uint8_t *dst_buf, const lv_area_t *dst_area, lv_color_format_t dst_cf,
        uint8_t *src_buf, const lv_image_header_t *src_header, const lv_area_t *zoom_area,
        const lv_area_t *draw_area, lv_opa_t opa);

int32_t sunxifb_g2d_blend(uint8_t *dst_buf, const lv_area_t *dst_area, lv_color_format_t dst_cf,
        uint8_t *src_buf, const lv_area_t *src_area, lv_color_format_t src_cf,
        const lv_area_t *draw_area, lv_opa_t opa, bool chroma_key);
#endif

/**********************
 *      MACROS
 **********************/

#endif  /*USE_SUNXIFB_G2D*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*SUNXIG2D_H*/
