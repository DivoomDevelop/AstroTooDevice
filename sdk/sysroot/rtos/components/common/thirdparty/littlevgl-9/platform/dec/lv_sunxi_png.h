/**
 * @file lv_sunxi_png.h
 *
 */

#ifndef LV_SUNXI_PNG_H
#define LV_SUNXI_PNG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#ifdef LV_USE_SUNXI_PNG

#include "lvgl.h"
#include "lvgl_private.h"

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
 * Register the PNG decoder functions in LVGL
 */
void lv_sunxi_png_init(void);
void lv_sunxi_png_deinit(void);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_SUNXI_PNG*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_SUNXI_PNG_H*/
