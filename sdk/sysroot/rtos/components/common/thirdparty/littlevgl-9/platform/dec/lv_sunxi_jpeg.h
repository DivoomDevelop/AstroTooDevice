/**
 * @file lv_sunxi_jpeg.h
 *
 */

#ifndef LV_SUNXI_JPEG_H
#define LV_SUNXI_JPEG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#ifdef LV_USE_SUNXI_JPEG

/*********************
 *      DEFINES
 *********************/
#ifdef LV_USE_SUNXI_JPEG_ZERO_COPY
#define LV_SUNXI_JPEG_COLOR_FORMAT LV_COLOR_FORMAT_I420
#else
#define LV_SUNXI_JPEG_COLOR_FORMAT LV_COLOR_FORMAT_RGB888
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Register the SUNXI JPEG decoder functions in LVGL
 */
void lv_sunxi_jpeg_init(void);

void lv_sunxi_jpeg_deinit(void);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_SUNXI_JPEG*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_SUNXI_JPEG_H*/
