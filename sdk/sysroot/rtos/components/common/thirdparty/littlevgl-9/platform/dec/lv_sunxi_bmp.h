/**
 * @file lv_bmp.h
 *
 */

#ifndef LV_SUNXI_BMP_H
#define LV_SUNXI_BMP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "src/lv_conf_internal.h"

#ifndef LV_USE_SUNXI_BMP
#define LV_USE_SUNXI_BMP   0
#endif

#if LV_USE_SUNXI_BMP

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void lv_sunxi_bmp_init(void);
void lv_sunxi_bmp_deinit(void);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_BMP*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_BMP_H*/
