#include "divoom_lvgl_flip_calendar.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct _divoom_lvgl_flip_calendar_demo_t
{
    lv_obj_t *root;
    lv_obj_t *calendar;
    lv_obj_t *caption;
    lv_timer_t *timer;
    int day;
    uint8_t first_flip;
} divoom_lvgl_flip_calendar_demo_t;

static void flip_calendar_demo_delete_event(lv_event_t *e)
{
    divoom_lvgl_flip_calendar_demo_t *demo;

    if (!e || lv_event_get_code(e) != LV_EVENT_DELETE)
        return;

    demo = (divoom_lvgl_flip_calendar_demo_t *)lv_event_get_user_data(e);
    if (!demo)
        return;

    if (demo->timer) {
        lv_timer_del(demo->timer);
        demo->timer = NULL;
    }
    free(demo);
}

static void flip_calendar_demo_timer_cb(lv_timer_t *timer)
{
    divoom_lvgl_flip_calendar_demo_t *demo;
    char buf[8];

    if (!timer)
        return;

    demo = (divoom_lvgl_flip_calendar_demo_t *)timer->user_data;
    if (!demo || !demo->calendar || !lv_obj_is_valid(demo->calendar))
        return;

    if (demo->first_flip) {
        demo->first_flip = 0;
        demo->day = 12;
        lv_timer_set_period(timer, 3000);
    }
    else {
        demo->day++;
        if (demo->day > 31)
            demo->day = 1;
    }

    snprintf(buf, sizeof(buf), "%02d", demo->day);
    divoom_lvgl_flip_calendar_flip_to(demo->calendar, buf);
}

void *divoom_lvgl_time_flip_demo_create(lv_obj_t *parent)
{
    divoom_lvgl_flip_calendar_demo_t *demo;
    divoom_lvgl_flip_calendar_style_t style;

    if (!parent)
        return NULL;

    demo = (divoom_lvgl_flip_calendar_demo_t *)calloc(1, sizeof(*demo));
    if (!demo)
        return NULL;

    demo->root = lv_obj_create(parent);
    if (!demo->root) {
        free(demo);
        return NULL;
    }

    lv_obj_remove_style_all(demo->root);
    lv_obj_clear_flag(demo->root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(demo->root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(demo->root, lv_color_hex(0x050708), 0);
    lv_obj_set_style_bg_opa(demo->root, LV_OPA_COVER, 0);
    lv_obj_add_event(demo->root, flip_calendar_demo_delete_event, LV_EVENT_DELETE, demo);

    demo->caption = lv_label_create(demo->root);
    if (demo->caption) {
        lv_label_set_text(demo->caption, "Flip Calendar Demo");
        lv_obj_set_style_text_color(demo->caption, lv_color_hex(0xC7A16A), 0);
        lv_obj_set_style_text_font(demo->caption, LV_FONT_DEFAULT, 0);
        lv_obj_align(demo->caption, LV_ALIGN_TOP_MID, 0, 54);
    }

    demo->calendar = divoom_lvgl_flip_calendar_create(demo->root);
    if (!demo->calendar)
        return demo;

    style.paper_color = lv_color_hex(0xEFE4D1);
    style.paper_shadow_color = lv_color_hex(0x000000);
    style.text_color = lv_color_hex(0x191817);
    style.center_line_color = lv_color_hex(0x9B8B74);
#if LV_FONT_MONTSERRAT_48
    style.font = &lv_font_montserrat_48;
#else
    style.font = LV_FONT_DEFAULT;
#endif
    style.radius = 12;
    style.duration_ms = 700;

    divoom_lvgl_flip_calendar_set_style(demo->calendar, &style);
    divoom_lvgl_flip_calendar_set_size(demo->calendar, 160, 120);
    divoom_lvgl_flip_calendar_set_text(demo->calendar, "11");
    lv_obj_align(demo->calendar, LV_ALIGN_CENTER, 0, 0);

    demo->day = 11;
    demo->first_flip = 1;
    demo->timer = lv_timer_create(flip_calendar_demo_timer_cb, 1000, demo);
    return demo;
}
