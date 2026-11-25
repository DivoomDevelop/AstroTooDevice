/**
 * @file lv_sunxi_draw_buf.h
 *
 */

#ifndef LV_SUNXI_DRAW_BUF_H
#define LV_SUNXI_DRAW_BUF_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#ifndef LV_USE_SUNXI_DRAW_BUF
#define LV_USE_SUNXI_DRAW_BUF   0
#endif

#if LV_USE_SUNXI_DRAW_BUF
#include "src/misc/lv_types.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
 * Register the SUNXI draw buf functions in LVGL
 */
void lv_sunxi_draw_buf_init(void);

void lv_sunxi_draw_buf_deinit(void);

lv_draw_buf_handlers_t* lv_sunxi_draw_buf_get_handler(void);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DRAW_BUF_JPEG*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_SUNXI_DRAW_BUF_H*/
