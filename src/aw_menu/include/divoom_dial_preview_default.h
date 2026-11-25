#ifndef DIVOOM_DIAL_PREVIEW_DEFAULT_H
#define DIVOOM_DIAL_PREVIEW_DEFAULT_H

#include <stdint.h>
#include "divoom_lvgl9_compat.h"

typedef struct
{
    lv_img_dsc_t image;
    uint8_t *pixels;
    uint32_t retry_at;
    uint8_t attempted;
} divoom_dial_preview_default;

/* LVGL thread only. Zero-initialize the owner and detach all image sources before release. */
const lv_img_dsc_t *divoom_dial_preview_default_acquire(divoom_dial_preview_default *preview);
void divoom_dial_preview_default_release(divoom_dial_preview_default *preview);

/* Compatibility entry point for the watchface bundle's legacy preview pages. */
const lv_img_dsc_t *divoom_dial_preview_default_image(void);

#endif
