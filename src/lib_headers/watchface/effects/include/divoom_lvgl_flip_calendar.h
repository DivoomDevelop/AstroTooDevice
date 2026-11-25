#ifndef DIVOOM_LVGL_FLIP_CALENDAR_H
#define DIVOOM_LVGL_FLIP_CALENDAR_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _divoom_lvgl_flip_calendar_style_t
{
    lv_color_t paper_color;
    lv_color_t paper_shadow_color;
    lv_color_t text_color;
    lv_color_t center_line_color;
    const lv_font_t *font;
    uint16_t radius;
    uint16_t duration_ms;
} divoom_lvgl_flip_calendar_style_t;

lv_obj_t *divoom_lvgl_flip_calendar_create(lv_obj_t *parent);
void divoom_lvgl_flip_calendar_set_size(lv_obj_t *obj, int w, int h);
void divoom_lvgl_flip_calendar_set_text(lv_obj_t *obj, const char *text);
void divoom_lvgl_flip_calendar_flip_to(lv_obj_t *obj, const char *next_text);
void divoom_lvgl_flip_calendar_set_style(lv_obj_t *obj, const divoom_lvgl_flip_calendar_style_t *style);

#ifdef __cplusplus
}
#endif

#endif
