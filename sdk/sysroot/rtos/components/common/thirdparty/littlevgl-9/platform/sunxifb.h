/**
 * @file sunxifb.h
 *
 */

#ifndef SUNXIFB_H
#define SUNXIFB_H

#ifdef __cplusplus
extern "C" {
#endif


#if USE_SUNXIFB

/*********************
 *      INCLUDES
 *********************/
#include <stdbool.h>
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
void sunxifb_init(void);
void sunxifb_exit(void);
void sunxifb_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *pixel_map);
void sunxifb_get_sizes(uint32_t *width, uint32_t *height);
void* sunxifb_alloc(size_t size, char *label);
void sunxifb_free(void **data, char *label);
void sunxifb_flush_cache(void *data, size_t size);
char* sunxifb_get_buf(void);
char* sunxifb_get_bufex(void);
int sunxifb_get_bits_per_pixel(void);
/* Horizontal mirror for FB blit (G2D_ROT_H). enable: 0 off, nonzero on. */
void sunxifb_set_mirror(int enable);
int sunxifb_get_mirror(void);
#ifdef USE_SUNXIFB_DOUBLE_BUFFER
bool sunxifb_get_dbuf_en(void);
int sunxifb_set_dbuf_en(lv_display_t *disp, bool dbuf_en);
#endif /* USE_SUNXIFB_DOUBLE_BUFFER */

/**********************
 *      MACROS
 **********************/

#endif  /*USE_SUNXIFB*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*SUNXIFB_H*/
