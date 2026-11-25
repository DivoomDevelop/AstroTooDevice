#ifndef _DIVOOM_ASTRO_CORE_SPECTRUM_H_
#define _DIVOOM_ASTRO_CORE_SPECTRUM_H_

#include "lvgl/lvgl.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DIVOOM_ASTRO_CORE_SPECTRUM_BANDS 16
#define DIVOOM_ASTRO_CORE_SPECTRUM_MAX_RAYS 48

typedef struct _divoom_astro_core_spectrum_cfg_t
{
    int center_x;
    int center_y;
    int inner_radius;
    int outer_radius;
    int ray_count;
    uint16_t refresh_ms;
    lv_color_t color_blue;
    lv_color_t color_pink;
} divoom_astro_core_spectrum_cfg_t;

lv_obj_t *divoom_astro_core_spectrum_create(lv_obj_t *parent,
    const divoom_astro_core_spectrum_cfg_t *cfg);
void divoom_astro_core_spectrum_set_bands(lv_obj_t *obj,
    const uint8_t bands[DIVOOM_ASTRO_CORE_SPECTRUM_BANDS],
    uint8_t eq_level);
void divoom_astro_core_spectrum_set_radius(lv_obj_t *obj,
    int inner_radius,
    int outer_radius);
void divoom_astro_core_spectrum_set_theme(lv_obj_t *obj,
    lv_color_t color_blue,
    lv_color_t color_pink);

#ifdef __cplusplus
}
#endif

#endif
