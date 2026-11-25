#ifndef _DIVOOM_ASTRO_CELL_GROWTH_H_
#define _DIVOOM_ASTRO_CELL_GROWTH_H_

#include "lvgl/lvgl.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DIVOOM_ASTRO_CELL_GROWTH_DEFAULT_W 480
#define DIVOOM_ASTRO_CELL_GROWTH_DEFAULT_H 480
#define DIVOOM_ASTRO_CELL_GROWTH_DEFAULT_MAX_CELLS 96

typedef struct _divoom_astro_cell_growth_cfg_t
{
    int width;
    int height;
    uint16_t refresh_ms;
    uint16_t max_cells;
    uint32_t seed;
    lv_color_t bg_color;
    lv_color_t teal_color;
    lv_color_t green_color;
    lv_color_t yellow_color;
    lv_color_t white_color;
} divoom_astro_cell_growth_cfg_t;

lv_obj_t *divoom_astro_cell_growth_create(lv_obj_t *parent,
    const divoom_astro_cell_growth_cfg_t *cfg);
void divoom_astro_cell_growth_reset(lv_obj_t *obj, uint32_t seed);
void divoom_astro_cell_growth_set_palette(lv_obj_t *obj,
    lv_color_t bg,
    lv_color_t teal,
    lv_color_t green,
    lv_color_t yellow,
    lv_color_t white);
void divoom_astro_cell_growth_set_paused(lv_obj_t *obj, int paused);
void divoom_astro_cell_growth_step(lv_obj_t *obj);

#ifdef __cplusplus
}
#endif

#endif
