/**
 * @file lv_sunxi_gif_private.h
 *
 */

#ifndef LV_SUNXI_GIF_PRIVATE_H
#define LV_SUNXI_GIF_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "src/widgets/image/lv_image_private.h"
#include "lv_sunxi_gif.h"

#if LV_USE_SUNXI_GIF

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

struct _lv_gif_t {
    lv_image_t img;
    gd_GIF * gif;
    lv_timer_t * timer;
    lv_image_dsc_t imgdsc;
    uint32_t last_call;
};


/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_SUNXI_GIF */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_SUNXI_GIF_PRIVATE_H*/
